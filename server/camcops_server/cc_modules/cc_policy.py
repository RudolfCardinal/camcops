#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_policy.py

===============================================================================

    Copyright (C) 2012-2020 Rudolf Cardinal (rudolf@pobox.com).

    This file is part of CamCOPS.

    CamCOPS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CamCOPS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CamCOPS. If not, see <https://www.gnu.org/licenses/>.

===============================================================================

**Represents ID number policies.**

Note that the upload script should NOT attempt to verify patients against the
ID policy, not least because tablets are allowed to upload task data (in a
separate transaction) before uploading patients; referential integrity would be
very hard to police. So the tablet software deals with ID compliance. (Also,
the superuser can change the server's ID policy retrospectively!)

Both the client and the server do policy tokenizing and can check patient info
against policies. The server has additional code to answer questions like "is
this policy valid?" (in general and in the context of the server's
configuration).

"""

import io
import logging
import tokenize
from typing import Callable, Dict, List, Optional, Tuple

from cardinal_pythonlib.dicts import reversedict
from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.reprfunc import auto_repr

from camcops_server.cc_modules.cc_simpleobjects import BarePatientInfo

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Tokens
# =============================================================================

TOKEN_TYPE = int
TOKENIZED_POLICY_TYPE = List[TOKEN_TYPE]

# http://stackoverflow.com/questions/36932
BAD_TOKEN = 0
TK_LPAREN = -1
TK_RPAREN = -2
TK_AND = -3
TK_OR = -4
TK_NOT = -5
TK_ANY_IDNUM = -6
TK_OTHER_IDNUM = -7
TK_FORENAME = -8
TK_SURNAME = -9
TK_SEX = -10
TK_DOB = -11
TK_ADDRESS = -12
TK_GP = -13
TK_OTHER_DETAILS = -14
TK_EMAIL = -15

# Tokens for ID numbers are from 1 upwards.

POLICY_TOKEN_DICT = {
    "(": TK_LPAREN,
    ")": TK_RPAREN,
    "AND": TK_AND,
    "OR": TK_OR,
    "NOT": TK_NOT,

    "ANYIDNUM": TK_ANY_IDNUM,
    "OTHERIDNUM": TK_OTHER_IDNUM,

    "FORENAME": TK_FORENAME,
    "SURNAME": TK_SURNAME,
    "SEX": TK_SEX,
    "DOB": TK_DOB,
    "ADDRESS": TK_ADDRESS,
    "GP": TK_GP,
    "OTHERDETAILS": TK_OTHER_DETAILS,
    "EMAIL": TK_EMAIL,
}
TOKEN_POLICY_DICT = reversedict(POLICY_TOKEN_DICT)

NON_IDNUM_INFO_TOKENS = [
    TK_OTHER_IDNUM, TK_ANY_IDNUM,
    TK_FORENAME, TK_SURNAME, TK_SEX, TK_DOB,
    TK_ADDRESS, TK_GP, TK_OTHER_DETAILS, TK_EMAIL,
]

TOKEN_IDNUM_PREFIX = "IDNUM"


def is_info_token(token: int) -> bool:
    """
    Is the token a kind that represents information, not (for example) an
    operator?
    """
    return token > 0 or token in NON_IDNUM_INFO_TOKENS


def token_to_str(token: int) -> str:
    """
    Returns a string version of the specified token.
    """
    if token < 0:
        return TOKEN_POLICY_DICT.get(token)
    else:
        return TOKEN_IDNUM_PREFIX + str(token)


# =============================================================================
# Quad-state logic
# =============================================================================

class QuadState(object):
    def __str__(self) -> str:
        if self is Q_TRUE:
            return "QTrue"
        elif self is Q_FALSE:
            return "QFalse"
        elif self is Q_DONT_CARE:
            return "QDontCare"
        else:
            return "QError"


Q_TRUE = QuadState()
Q_FALSE = QuadState()
Q_ERROR = QuadState()
Q_DONT_CARE = QuadState()


def bool_to_quad(x: bool) -> QuadState:
    return Q_TRUE if x else Q_FALSE


def quad_not(x: QuadState) -> QuadState:
    # Boolean logic
    if x is Q_TRUE:
        return Q_FALSE
    elif x is Q_FALSE:
        return Q_TRUE
    # Unusual logic
    elif x is Q_DONT_CARE:
        return Q_DONT_CARE
    else:
        return Q_ERROR


def quad_and(x: QuadState, y: QuadState) -> QuadState:
    either = (x, y)
    # Unusual logic
    if Q_ERROR in either:
        return Q_ERROR
    elif Q_DONT_CARE in either:
        other = either[1] if either[0] == Q_DONT_CARE else either[0]
        return other
    # Boolean logic
    elif x is Q_TRUE and y is Q_TRUE:
        return Q_TRUE
    else:
        return Q_FALSE


def quad_or(x: QuadState, y: QuadState) -> QuadState:
    either = (x, y)
    # Unusual logic
    if Q_ERROR in either:
        return Q_ERROR
    elif Q_DONT_CARE in either:
        other = either[1] if either[0] == Q_DONT_CARE else either[0]
        return other
    # Boolean logic
    elif x is Q_TRUE or y is Q_TRUE:
        return Q_TRUE
    else:
        return Q_FALSE


def debug_wrapper(fn: Callable, name: str) -> Callable:
    def wrap(*args, **kwargs) -> QuadState:
        result = fn(*args, **kwargs)
        arglist = [str(x) for x in args] + [f"{k}={v}"
                                            for k, v in kwargs.items()]
        log.critical("{}({}) -> {}".format(name, ", ".join(arglist), result))
        return result
    return wrap


DEBUG_QUAD_STATE_LOGIC = False

if DEBUG_QUAD_STATE_LOGIC:
    quad_not = debug_wrapper(quad_not, "quad_not")
    quad_and = debug_wrapper(quad_and, "quad_and")
    quad_or = debug_wrapper(quad_or, "quad_or")


# =============================================================================
# PatientInfoPresence
# =============================================================================

class PatientInfoPresence(object):
    """
    Represents simply the presence/absence of different kinds of information
    about a patient.
    """
    def __init__(self,
                 present: Dict[int, QuadState] = None,
                 default: QuadState = Q_FALSE) -> None:
        """
        Args:
            present: map from token to :class:`QuadState`
            default: default :class:`QuadState` to return if unspecified
        """
        self.present = present or {}  # type: Dict[int, QuadState]
        self.default = default
        for t in self.present.keys():
            assert is_info_token(t)

    def __repr__(self) -> str:
        return auto_repr(self)

    def is_present(self, token: int,
                   default: QuadState = None) -> QuadState:
        """
        Is information represented by a particular token present?

        Args:
            token: token to check for; e.g. :data:`TK_FORENAME`
            default: default :class:`QuadState` to return if unspecified; if
                this is None, ``self.default`` is used.

        Returns:
            a :class:`QuadState`
        """
        return self.present.get(token, default or self.default)

    @property
    def forename_present(self) -> QuadState:
        return self.is_present(TK_FORENAME)

    @property
    def surname_present(self) -> QuadState:
        return self.is_present(TK_SURNAME)

    @property
    def sex_present(self) -> QuadState:
        return self.is_present(TK_SEX)

    @property
    def dob_present(self) -> QuadState:
        return self.is_present(TK_DOB)

    @property
    def address_present(self) -> QuadState:
        return self.is_present(TK_ADDRESS)

    @property
    def email_present(self) -> QuadState:
        return self.is_present(TK_EMAIL)

    @property
    def gp_present(self) -> QuadState:
        return self.is_present(TK_GP)

    @property
    def otherdetails_present(self) -> QuadState:
        return self.is_present(TK_OTHER_DETAILS)

    @property
    def otheridnum_present(self) -> QuadState:
        return self.is_present(TK_OTHER_IDNUM)

    @property
    def special_anyidnum_present(self) -> QuadState:
        return self.is_present(TK_ANY_IDNUM)

    def idnum_present(self, which_idnum: int) -> QuadState:
        """
        Is the specified ID number type present?
        """
        assert which_idnum > 0
        return self.is_present(which_idnum)

    def any_idnum_present(self) -> QuadState:
        """
        Is at least one ID number present?
        """
        for k, v in self.present.items():
            if k > 0 and v is Q_TRUE:
                return Q_TRUE
        return self.special_anyidnum_present

    @classmethod
    def make_from_ptinfo(
            cls,
            ptinfo: BarePatientInfo,
            policy_mentioned_idnums: List[int]) -> "PatientInfoPresence":
        """
        Returns a :class:`PatientInfoPresence` representing whether different
        kinds of information about the patient are present or not.
        """
        presences = {
            TK_FORENAME: bool_to_quad(bool(ptinfo.forename)),
            TK_SURNAME: bool_to_quad(bool(ptinfo.surname)),
            TK_SEX: bool_to_quad(bool(ptinfo.sex)),
            TK_DOB: bool_to_quad(ptinfo.dob is not None),
            TK_ADDRESS: bool_to_quad(bool(ptinfo.address)),
            TK_EMAIL: bool_to_quad(bool(ptinfo.email)),
            TK_GP: bool_to_quad(bool(ptinfo.gp)),
            TK_OTHER_DETAILS: bool_to_quad(bool(ptinfo.otherdetails)),
            TK_OTHER_IDNUM: Q_FALSE,  # may change
        }  # type: Dict[int, QuadState]
        for iddef in ptinfo.idnum_definitions:
            this_idnum_present = iddef.idnum_value is not None
            presences[iddef.which_idnum] = bool_to_quad(this_idnum_present)
            if iddef.which_idnum not in policy_mentioned_idnums:
                presences[TK_OTHER_IDNUM] = Q_TRUE
        return cls(presences, default=Q_FALSE)

    @classmethod
    def make_uncaring(cls) -> "PatientInfoPresence":
        """
        Makes a :class:`PatientInfoPresence` that doesn't care about anything.
        """
        return cls({}, default=Q_DONT_CARE)

    def set_idnum_presence(self, which_idnum: int, present: QuadState) -> None:
        """
        Set the "presence" state for one ID number type.

        Args:
            which_idnum: which ID number type
            present: its state of being present (or not, or other states)
        """
        self.present[which_idnum] = present

    @classmethod
    def make_uncaring_except(cls, token: int,
                             present: QuadState) -> "PatientInfoPresence":
        """
        Make a :class:`PatientInfoPresence` that is uncaring except for one
        thing, specified by token.
        """
        assert is_info_token(token)
        pip = cls.make_uncaring()
        pip.present[token] = present
        return pip


# =============================================================================
# More constants
# =============================================================================

CONTENT_TOKEN_PROCESSOR_TYPE = Callable[[int], QuadState]


# =============================================================================
# TokenizedPolicy
# =============================================================================

class TokenizedPolicy(object):
    """
    Represents a tokenized ID policy.

    A tokenized policy is a policy represented by a sequence of integers;
    0 means "bad token"; negative numbers represent fixed things like
    "forename" or "left parenthesis" or "and"; positive numbers represent
    ID number types.
    """
    def __init__(self, policy: str) -> None:
        self.tokens = self.get_tokenized_id_policy(policy)
        self._syntactically_valid = None  # type: Optional[bool]
        self.valid_idnums = None  # type: Optional[List[int]]
        self._valid_for_idnums = None  # type: Optional[bool]

    def __str__(self) -> str:
        policy = " ".join(token_to_str(t) for t in self.tokens)
        policy = policy.replace("( ", "(")
        policy = policy.replace(" )", ")")
        return policy

    # -------------------------------------------------------------------------
    # ID number info
    # -------------------------------------------------------------------------

    def set_valid_idnums(self, valid_idnums: List[int]) -> None:
        """
        Make a note of which ID number types are currently valid.
        Caches "valid for these ID numbers" information.

        Args:
            valid_idnums: list of valid ID number types
        """
        sorted_idnums = sorted(valid_idnums)
        if sorted_idnums != self.valid_idnums:
            self.valid_idnums = sorted_idnums
            self._valid_for_idnums = None  # clear cache

    def require_valid_idnum_info(self) -> None:
        """
        Checks that set_valid_idnums() has been called properly, or raises
        :exc:`AssertionError`.
        """
        assert self.valid_idnums is not None, (
            "Must call set_valid_idnums() first! Currently: {!r}"
        )

    # -------------------------------------------------------------------------
    # Tokenize
    # -------------------------------------------------------------------------

    @staticmethod
    def name_to_token(name: str) -> int:
        """
        Converts an upper-case string token name (such as ``DOB``) to an
        integer token.
        """
        if name in POLICY_TOKEN_DICT:
            return POLICY_TOKEN_DICT[name]
        if name.startswith(TOKEN_IDNUM_PREFIX):
            nstr = name[len(TOKEN_IDNUM_PREFIX):]
            try:
                return int(nstr)
            except (TypeError, ValueError):
                return BAD_TOKEN
        return BAD_TOKEN

    @classmethod
    def get_tokenized_id_policy(cls, policy: str) \
            -> TOKENIZED_POLICY_TYPE:
        """
        Takes a string policy and returns a tokenized policy, meaning a list of
        integer tokens, or ``[]``.
        """
        if policy is None:
            return []
        # http://stackoverflow.com/questions/88613
        string_index = 1
        # single line, upper case:
        policy = " ".join(policy.strip().upper().splitlines())
        try:
            tokenstrings = list(
                token[string_index]
                for token in tokenize.generate_tokens(
                    io.StringIO(policy).readline)
                if token[string_index]
            )
        except tokenize.TokenError:
            # something went wrong
            return []
        tokens = [cls.name_to_token(k) for k in tokenstrings]
        if any(t == BAD_TOKEN for t in tokens):
            # There's something bad in there.
            return []
        return tokens

    # -------------------------------------------------------------------------
    # Validity checks
    # -------------------------------------------------------------------------

    def is_syntactically_valid(self) -> bool:
        """
        Is the policy syntactically valid? This is a basic check.
        """
        if self._syntactically_valid is None:
            # Cache it
            if not self.tokens:
                self._syntactically_valid = False
            else:
                # Evaluate against a dummy patient info object. If we get None,
                # it's gone wrong.
                pip = PatientInfoPresence()
                value = self._value_for_pip(pip)
                self._syntactically_valid = value is not Q_ERROR
        return self._syntactically_valid

    def is_valid(self, valid_idnums: List[int] = None,
                 verbose: bool = False) -> bool:
        """
        Is the policy valid in the context of the ID types available in our
        database?

        Args:
            valid_idnums: optional list of valid ID number types
            verbose: report reasons to debug log
        """
        if valid_idnums is not None:
            self.set_valid_idnums(valid_idnums)
        if self._valid_for_idnums is None:
            # Cache information
            self.require_valid_idnum_info()
            self._valid_for_idnums = self.is_valid_for_idnums(
                self.valid_idnums, verbose=verbose)
        return self._valid_for_idnums

    def is_valid_for_idnums(self, valid_idnums: List[int],
                            verbose: bool = False) -> bool:
        """
        Is the policy valid, given a list of valid ID number types?

        Checks the following:

        - valid syntax
        - refers only to ID number types defined on the server
        - is compatible with the tablet ID policy

        Args:
            valid_idnums: ID number types that are valid on the server
            verbose: report reasons to debug log
        """
        # First, syntax:
        if not self.is_syntactically_valid():
            if verbose:
                log.debug("is_valid_for_idnums(): Not syntactically valid")
            return False
        # Second, all ID numbers referred to by the policy exist:
        for token in self.tokens:
            if token > 0 and token not in valid_idnums:
                if verbose:
                    log.debug("is_valid_for_idnums(): Refers to ID number type "
                              "{}, which does not exist", token)
                return False
        if not self._compatible_with_tablet_id_policy(verbose=verbose):
            if verbose:
                log.debug("is_valid_for_idnums(): Less restrictive than the "
                          "tablet minimum ID policy; invalid")
            return False
        return True

    # -------------------------------------------------------------------------
    # Information about the ID number types the policy refers to
    # -------------------------------------------------------------------------

    def relevant_idnums(self, valid_idnums: List[int]) -> List[int]:
        """
        Which ID numbers are relevant to this policy?

        Args:
            valid_idnums: ID number types that are valid on the server

        Returns:
            the subset of ``valid_idnums`` that is mentioned somehow in the
            policy
        """
        if not self.tokens:
            return []
        if TK_ANY_IDNUM in self.tokens or TK_OTHER_IDNUM in self.tokens:
            # all are relevant
            return valid_idnums
        relevant_idnums = []  # type: List[int]
        for which_idnum in valid_idnums:
            assert which_idnum > 0, "Silly ID number types"
            if which_idnum in self.tokens:
                relevant_idnums.append(which_idnum)
        return relevant_idnums

    def specifically_mentioned_idnums(self) -> List[int]:
        """
        Returns the ID number tokens for all ID numbers mentioned in the
        policy, as a list.
        """
        return [x for x in self.tokens if x > 0]

    def contains_specific_idnum(self, which_idnum: int) -> bool:
        """
        Does the policy refer specifically to the given ID number type?

        Args:
            which_idnum: ID number type to test
        """
        assert which_idnum > 0
        return which_idnum in self.tokens

    # -------------------------------------------------------------------------
    # More complex attributes
    # -------------------------------------------------------------------------

    def find_critical_single_numerical_id(
            self,
            valid_idnums: List[int] = None,
            verbose: bool = False) -> Optional[int]:
        """
        If the policy involves a single mandatory ID number, return that ID
        number; otherwise return None.

        Args:
            valid_idnums: ID number types that are valid on the server
            verbose: report reasons to debug log

        Returns:
            int: the single critical ID number type, or ``None``
        """
        if not self.is_valid(valid_idnums):
            if verbose:
                log.debug("find_critical_single_numerical_id(): invalid")
            return None
        relevant_idnums = self.specifically_mentioned_idnums()
        possible_critical_idnums = []  # type: List[int]
        for which_idnum in relevant_idnums:
            pip_with = PatientInfoPresence.make_uncaring_except(which_idnum,
                                                                Q_TRUE)
            satisfies_with_1 = self._value_for_pip(pip_with)
            pip_with.present[TK_OTHER_IDNUM] = Q_FALSE
            satisfies_with_2 = self._value_for_pip(pip_with)
            pip_without = PatientInfoPresence.make_uncaring_except(which_idnum,
                                                                   Q_FALSE)
            satisfies_without_1 = self._value_for_pip(pip_without)
            pip_with.present[TK_OTHER_IDNUM] = Q_TRUE
            satisfies_without_2 = self._value_for_pip(pip_without)
            if verbose:
                log.debug(
                    "... {}: satisfies_with={}, satisfies_without_1={}, "
                    "satisfies_without_2={}",
                    which_idnum, satisfies_with_1, satisfies_without_1,
                    satisfies_without_2,
                )
            if (satisfies_with_1 is Q_TRUE and
                    satisfies_with_2 is Q_TRUE and
                    satisfies_without_1 is Q_FALSE and
                    satisfies_without_2 is Q_FALSE):
                possible_critical_idnums.append(which_idnum)
        if verbose:
            log.debug(
                "find_critical_single_numerical_id(): "
                "possible_critical_idnums = {}",
                possible_critical_idnums)
        if len(possible_critical_idnums) == 1:
            return possible_critical_idnums[0]
        return None

    def is_idnum_mandatory_in_policy(
            self,
            which_idnum: int,
            valid_idnums: List[int],
            verbose: bool = False) -> bool:
        """
        Is the ID number mandatory in the specified policy?

        Args:
            which_idnum: ID number type to test
            valid_idnums: ID number types that are valid on the server
            verbose: report reasons to debug log
        """
        if which_idnum is None or which_idnum < 1:
            if verbose:
                log.debug("is_idnum_mandatory_in_policy(): bad ID type")
            return False
        if not self.contains_specific_idnum(which_idnum):
            if verbose:
                log.debug("is_idnum_mandatory_in_policy(): policy does not "
                          "contain ID {}, so not mandatory", which_idnum)
            return False
        self.set_valid_idnums(valid_idnums)
        if not self.is_valid():
            if verbose:
                log.debug("is_idnum_mandatory_in_policy(): policy invalid")
            return False

        pip_with = PatientInfoPresence.make_uncaring_except(which_idnum,
                                                            Q_TRUE)
        satisfies_with = self._value_for_pip(pip_with)
        if satisfies_with != Q_TRUE:
            if verbose:
                log.debug("is_idnum_mandatory_in_policy(): policy not "
                          "satisfied by presence of ID {}, so not mandatory",
                          which_idnum)
            return False
        pip_without = PatientInfoPresence.make_uncaring_except(which_idnum,
                                                               Q_FALSE)
        satisfies_without = self._value_for_pip(pip_without)
        if satisfies_without != Q_FALSE:
            if verbose:
                log.debug("is_idnum_mandatory_in_policy(): policy satisfied "
                          "without presence of ID {}, so not mandatory",
                          which_idnum)
            return False
        # Thus, if we get here, the policy is unhappy with the absence of our
        # ID number type, but happy with it; therefore it is mandatory.
        if verbose:
            log.debug("is_idnum_mandatory_in_policy(): ID {} is mandatory",
                      which_idnum)
        return True

    def _requires_prohibits(self, token: int,
                            verbose: bool = False) -> Tuple[bool, bool]:
        """
        Does this policy require, and/or prohibit, a particular token?

        Args:
            token: token to check
            verbose: report reasons to debug log

        Returns:
            tuple: requires, prohibits
        """
        pip_with = PatientInfoPresence.make_uncaring_except(token, Q_TRUE)
        satisfies_with = self._value_for_pip(pip_with)
        pip_without = PatientInfoPresence.make_uncaring_except(token, Q_FALSE)
        satisfies_without = self._value_for_pip(pip_without)
        requires = (
            satisfies_with is Q_TRUE and
            satisfies_without is Q_FALSE
        )
        prohibits = (
            satisfies_with is Q_FALSE and
            satisfies_without is Q_TRUE
        )
        if verbose:
            log.debug(
                "_requires_prohibits({t}): "
                "satisfies_with={sw}, "
                "satisfies_without={swo}, "
                "requires={r}, "
                "prohibits={p}",
                t=token_to_str(token),
                sw=satisfies_with,
                swo=satisfies_without,
                r=requires,
                p=prohibits,
            )
        return requires, prohibits

    def _requires_sex(self, verbose: bool = False) -> bool:
        """
        Does this policy require sex to be present?

        Args:
            verbose: report reasons to debug log
        """
        requires, _ = self._requires_prohibits(TK_SEX, verbose=verbose)
        return requires

    def _requires_an_idnum(self, verbose: bool = False) -> bool:
        """
        Does this policy require an ID number to be present?

        Args:
            verbose: report reasons to debug log
        """
        if verbose:
            log.debug("_requires_an_idnum():")
        for token in self.specifically_mentioned_idnums() + [TK_ANY_IDNUM,
                                                             TK_OTHER_IDNUM]:
            requires, _ = self._requires_prohibits(token, verbose=verbose)
            if requires:
                if verbose:
                    log.debug("... requires ID number '{}'",
                              token_to_str(token))
                return True
        return False

    # def _less_restrictive_than(self, other: "TokenizedPolicy",
    #                            valid_idnums: List[int],
    #                            verbose: bool = False) -> bool:
    #     """
    #     Is this ("self") policy less restrictive than the "other" policy?
    #
    #     "More restrictive" means "requires more information".
    #     "Less restrictive" means "requires or enforces less information".
    #
    #     Therefore, we must return True if we can find a situation where "self"
    #     is satisfied but "other" is not.
    #
    #     Args:
    #         other: the other policy
    #         valid_idnums: ID number types that are valid on the server
    #         verbose: report reasons to debug log
    #
    #     This is very difficult. Abandoned this generic attempt in favour of a
    #     specific hard-coded check for the tablet policy.
    #     """
    #     if verbose:
    #         log.debug("_less_restrictive_than(): self={}, other={}",
    #                   self, other)
    #     possible_tokens = valid_idnums + NON_IDNUM_INFO_TOKENS
    #     for token in possible_tokens:
    #         # Self
    #         self_requires, self_prohibits = self._requires_prohibits(
    #             token, valid_idnums)
    #         # Other
    #         pip_with = PatientInfoPresence.make_uncaring_except(
    #             token, Q_TRUE, valid_idnums)
    #         other_satisfies_with = other._value_for_pip(pip_with)
    #         pip_without = PatientInfoPresence.make_uncaring_except(
    #             token, Q_FALSE, valid_idnums)
    #         other_satisfies_without_1 = other._value_for_pip(pip_without)
    #         pip_without.special_anyidnum_present = Q_TRUE
    #         other_satisfies_without_2 = other._value_for_pip(pip_without)
    #         other_requires = (
    #             other_satisfies_with is Q_TRUE and
    #             other_satisfies_without_1 is Q_FALSE and
    #             other_satisfies_without_2 is Q_FALSE
    #         )
    #         other_prohibits = (
    #             other_satisfies_with is Q_FALSE and
    #             other_satisfies_without_1 is Q_TRUE and
    #             other_satisfies_without_2 is Q_TRUE
    #         )
    #         if verbose:
    #             log.debug(
    #                 "... for {t}: "
    #                 "self_requires={sr}, "
    #                 "self_prohibits={sp}, "
    #                 "other_satisfies_with={osw}, "
    #                 "other_satisfies_without_1={oswo1}, "
    #                 "other_satisfies_without_2={oswo2}, "
    #                 "other_requires={or_}",
    #                 "other_prohibits={op}",
    #                 t=token_to_str(token),
    #                 sr=self_requires,
    #                 sp=self_prohibits,
    #                 osw=other_satisfies_with,
    #                 oswo1=other_satisfies_without_1,
    #                 oswo2=other_satisfies_without_2,
    #                 or_=other_requires,
    #                 op=other_prohibits,
    #             )
    #
    #         if other_requires and not self_requires:
    #             # The "self" policy is LESS RESTRICTIVE (requires less info).
    #             if verbose:
    #                 log.debug(
    #                     "... self does not require ID type {}, but other does "  # noqa
    #                     "require it; therefore self is less restrictive",
    #                     token)
    #             return True
    #         # if self_prohibits and not other_prohibits:
    #         #     # The "self" policy is LESS RESTRICTIVE (enforces less info).  # noqa
    #         #     if verbose:
    #         #         log.debug(
    #         #             "... self prohibits ID type {}, but other does not "  # noqa
    #         #             "prohibit it; therefore self is less restrictive",
    #         #             token)
    #         #     return True
    #     if verbose:
    #         log.debug(
    #             "... by elimination, self [{}] not less "
    #             "restrictive than other [{}]",
    #             self, other
    #         )
    #     return False

    def _compatible_with_tablet_id_policy(self,
                                          verbose: bool = False) -> bool:
        """
        Is this policy compatible with :data:`TABLET_ID_POLICY`?

        The "self" policy may be MORE restrictive than the tablet minimum ID
        policy, but may not be LESS restrictive.

        Args:
            verbose: report reasons to debug log

        Internal function -- doesn't used cached information.
        """
        # Method 1: abandoned.
        # We previously used a version of _less_restrictive_than() that
        # did a brute-force attempt, but that became prohibitive as ID numbers
        # got added.
        # A generic method is very hard (see above) -- not properly succeeded
        # yet.
        #
        # return not self._less_restrictive_than(
        #     TABLET_ID_POLICY, valid_idnums, verbose=verbose)

        # Method 2: manual.
        if verbose:
            log.debug("_compatible_with_tablet_id_policy():")
        requires_sex = self._requires_sex(verbose=verbose)
        if requires_sex:
            if verbose:
                log.debug("... requires sex")
        else:
            if verbose:
                log.debug("... doesn't require sex; returning False")
            return False
        requires_an_idnum = self._requires_an_idnum(verbose=verbose)
        if requires_an_idnum:
            if verbose:
                log.debug("... requires an ID number; returning True")
            return True
        if verbose:
            log.debug("... does not require an ID number; trying alternatives")
        other_mandatory = [TK_FORENAME, TK_SURNAME, TK_DOB, TK_EMAIL]
        for token in other_mandatory:
            requires, _ = self._requires_prohibits(token, verbose=verbose)
            if not requires:
                if verbose:
                    log.debug("... does not require '{}'; returning False",
                              token_to_str(token))
                return False
        log.debug("... requires all of {!r}; returning True",
                  [token_to_str(t) for t in other_mandatory])
        return True

    def compatible_with_tablet_id_policy(self,
                                         valid_idnums: List[int],
                                         verbose: bool = False) -> bool:
        """
        Is this policy compatible with :data:`TABLET_ID_POLICY`?

        The "self" policy may be MORE restrictive than the tablet minimum ID
        policy, but may not be LESS restrictive.

        Args:
            valid_idnums: ID number types that are valid on the server
            verbose: report reasons to debug log
        """
        self.set_valid_idnums(valid_idnums)
        if not self.is_valid(verbose=verbose):
            return False
        return self._compatible_with_tablet_id_policy(verbose=verbose)

    # -------------------------------------------------------------------------
    # Check if a patient satisfies the policy
    # -------------------------------------------------------------------------

    def _value_for_ptinfo(self, ptinfo: BarePatientInfo) -> QuadState:
        """
        What does the policy evaluate to for a given patient info object?

        Args:
            ptinfo:
                a `camcops_server.cc_modules.cc_simpleobjects.BarePatientInfo`

        Returns:
            a :class:`QuadState` quad-state value
        """
        pip = PatientInfoPresence.make_from_ptinfo(
            ptinfo,
            self.specifically_mentioned_idnums()
        )
        return self._value_for_pip(pip)

    def _value_for_pip(self, pip: PatientInfoPresence) -> QuadState:
        """
        What does the policy evaluate to for a given patient info presence
        object?

        Args:
            pip:
                a `camcops_server.cc_modules.cc_simpleobjects.PatientInfoPresence`

        Returns:
            a :class:`QuadState` quad-state value
        """  # noqa
        def content_token_processor(token: int) -> QuadState:
            return self._element_value_test_pip(pip, token)

        return self._chunk_value(
            self.tokens,
            content_token_processor=content_token_processor)
        # ... which is recursive

    def satisfies_id_policy(self, ptinfo: BarePatientInfo) -> bool:
        """
        Does the patient information in ptinfo satisfy the specified ID policy?

        Args:
            ptinfo:
                a `camcops_server.cc_modules.cc_simpleobjects.BarePatientInfo`
        """
        return self._value_for_ptinfo(ptinfo) is Q_TRUE

    # -------------------------------------------------------------------------
    # Functions for the policy to parse itself and compare itself to a patient
    # -------------------------------------------------------------------------

    def _chunk_value(self,
                     tokens: TOKENIZED_POLICY_TYPE,
                     content_token_processor: CONTENT_TOKEN_PROCESSOR_TYPE) \
            -> QuadState:
        """
        Applies the policy to the patient info in ``ptinfo``.
        Can be used recursively.

        Args:
            tokens:
                a tokenized policy
            content_token_processor:
                a function to be called for each "content" token, which returns
                its Boolean value, or ``None`` in case of failure

        Returns:
            a :class:`QuadState` quad-state value
        """
        want_content = True
        processing_and = False
        processing_or = False
        index = 0
        value = None  # type: Optional[QuadState]
        while index < len(tokens):
            if want_content:
                nextchunk, index = self._content_chunk_value(
                    tokens, index, content_token_processor)
                if nextchunk is Q_ERROR:
                    return Q_ERROR  # fail
                if value is None:
                    value = nextchunk
                elif processing_and:
                    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    # implement logical AND
                    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    value = quad_and(value, nextchunk)
                elif processing_or:
                    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    # implement logical OR
                    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    value = quad_or(value, nextchunk)
                else:
                    # Error; shouldn't get here
                    return Q_ERROR
                processing_and = False
                processing_or = False
            else:
                # Want operator
                operator, index = self._op(tokens, index)
                if operator is None:
                    return Q_ERROR  # fail
                if operator == TK_AND:
                    processing_and = True
                elif operator == TK_OR:
                    processing_or = True
                else:
                    # Error; shouldn't get here
                    return Q_ERROR
            want_content = not want_content
        if want_content:
            log.debug("_chunk_value(): ended wanting content; bad policy")
            return Q_ERROR
        return value

    def _content_chunk_value(
            self,
            tokens: TOKENIZED_POLICY_TYPE,
            start: int,
            content_token_processor: CONTENT_TOKEN_PROCESSOR_TYPE) \
            -> Tuple[QuadState, int]:
        """
        Applies part of a policy to ``ptinfo``. The part of policy pointed to
        by ``start`` represents something -- "content" -- that should return a
        value (not an operator, for example). Called by :func:`id_policy_chunk`
        (q.v.).

        Args:
            tokens:
                a tokenized policy (list of integers)
            start:
                zero-based index of the first token to check
            content_token_processor:
                a function to be called for each "content" token, which returns
                its Boolean value, or ``None`` in case of failure

        Returns:
            tuple: chunk_value, next_index. ``chunk_value`` is ``True`` if the
            specified chunk is satisfied by the ``ptinfo``, ``False`` if it
            isn't, and ``None`` if there was an error. ``next_index`` is the
            index of the next token after this chunk.

        """
        if start >= len(tokens):
            log.debug("_content_chunk_value(): "
                      "beyond end of policy; bad policy")
            return Q_ERROR, start
        token = tokens[start]
        if token in [TK_RPAREN, TK_AND, TK_OR]:
            log.debug("_content_chunk_value(): "
                      "chunk starts with ), AND, or OR; bad policy")
            return Q_ERROR, start
        elif token == TK_LPAREN:
            # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            # implement parentheses
            # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            subchunkstart = start + 1  # exclude the opening bracket
            # Find closing parenthesis
            depth = 1
            searchidx = subchunkstart
            while depth > 0:
                if searchidx >= len(tokens):
                    log.debug("_content_chunk_value(): "
                              "Unmatched left parenthesis; bad policy")
                    return Q_ERROR, start
                elif tokens[searchidx] == TK_LPAREN:
                    depth += 1
                elif tokens[searchidx] == TK_RPAREN:
                    depth -= 1
                searchidx += 1
            subchunkend = searchidx - 1
            # ... to exclude the closing bracket from the analysed subchunk
            chunk_value = self._chunk_value(
                tokens[subchunkstart:subchunkend], content_token_processor)
            return chunk_value, subchunkend + 1  # to move past the closing bracket  # noqa
        elif token == TK_NOT:
            next_value, next_index = self._content_chunk_value(
                tokens, start + 1, content_token_processor)
            if next_value is Q_ERROR:
                return next_value, start
            # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            # implement logical NOT
            # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            return quad_not(next_value), next_index
        else:
            # meaningful token
            return content_token_processor(token), start + 1

    @classmethod
    def _op(cls, policy: TOKENIZED_POLICY_TYPE, start: int) \
            -> Tuple[Optional[TOKEN_TYPE], int]:
        """
        Returns an operator from the policy, beginning at index ``start``, or
        ``None`` if there wasn't an operator there.

            policy:
                a tokenized policy (list of integers)
            start:
                zero-based index of the first token to check

        Returns:
            tuple: ``operator, next_index``. ``operator`` is the operator's
            integer token or ``None``. ``next_index`` gives the next index of
            the policy to check at.
        """
        if start >= len(policy):
            log.debug("_op(): beyond end of policy")
            return None, start
        token = policy[start]
        if token in [TK_AND, TK_OR]:
            return token, start + 1
        else:
            log.debug("_op(): not an operator; bad policy")
            # Not an operator
            return None, start

    # Things to do with content tokens 1: are they present in patient info?

    @staticmethod
    def _element_value_test_pip(pip: PatientInfoPresence,
                                token: TOKEN_TYPE) -> QuadState:
        """
        Returns the "value" of a content token as judged against the patient
        information. For example, if the patient information contains a date of
        birth, a ``TK_DOB`` token will evaluate to ``True``.

        Args:
            pip:
                a `camcops_server.cc_modules.cc_simpleobjects.PatientInfoPresence`
            token:
                an integer token from the policy

        Returns:
            a :class:`QuadState` quad-state value
        """  # noqa
        assert is_info_token(token)
        if token == TK_ANY_IDNUM:
            return pip.any_idnum_present()
        else:
            return pip.is_present(token)


# =============================================================================
# Tablet ID policy
# =============================================================================

TABLET_ID_POLICY_STR = "sex AND ((forename AND surname AND dob) OR anyidnum)"
TABLET_ID_POLICY = TokenizedPolicy(TABLET_ID_POLICY_STR)
