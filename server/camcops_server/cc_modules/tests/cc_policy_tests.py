#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_policy_tests.py

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
"""

import logging
from typing import Dict

from cardinal_pythonlib.logs import BraceStyleAdapter
from pendulum import Date

from camcops_server.cc_modules.cc_policy import TokenizedPolicy
from camcops_server.cc_modules.cc_simpleobjects import (
    BarePatientInfo,
    IdNumReference,
)
from camcops_server.cc_modules.cc_unittest import ExtendedTestCase

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Unit tests
# =============================================================================

class PolicyTests(ExtendedTestCase):
    """
    Unit tests.
    """
    def test_policies(self) -> None:
        self.announce("test_policies")

        class TestRig(object):
            def __init__(
                    self,
                    policy: str,
                    syntactically_valid: bool = None,
                    valid: bool = None,
                    ptinfo_satisfies_id_policy: bool = None,
                    test_critical_single_numerical_id: bool = False,
                    critical_single_numerical_id: int = None,
                    compatible_with_tablet_id_policy: bool = None,
                    is_idnum_mandatory_in_policy: Dict[int, bool] = None) \
                    -> None:
                self.policy = policy
                self.syntactically_valid = syntactically_valid
                self.valid = valid
                self.ptinfo_satisfies_id_policy = ptinfo_satisfies_id_policy
                self.test_critical_single_numerical_id = test_critical_single_numerical_id  # noqa
                self.critical_single_numerical_id = critical_single_numerical_id  # noqa
                self.compatible_with_tablet_id_policy = compatible_with_tablet_id_policy  # noqa
                self.is_idnum_mandatory_in_policy = is_idnum_mandatory_in_policy or {}  # type: Dict[int, bool]  # noqa

        valid_idnums = list(range(1, 10 + 1))
        # noinspection PyTypeChecker
        bpi = BarePatientInfo(
            forename="forename",
            surname="surname",
            dob=Date.today(),  # random value
            email="patient@example.com",
            sex="F",
            idnum_definitions=[
                IdNumReference(1, 1),
                IdNumReference(10, 3),
            ],
        )
        test_policies = [
            TestRig(
                "",
                syntactically_valid=False,
                valid=False,
            ),
            TestRig(
                "sex AND (failure",
                syntactically_valid=False,
                valid=False,
            ),
            TestRig(
                "sex AND NOT",
                syntactically_valid=False,
                valid=False,
            ),
            TestRig(
                "OR OR",
                syntactically_valid=False,
                valid=False,
            ),
            TestRig(
                "idnum99",
                syntactically_valid=True,
                valid=False,
            ),
            TestRig(
                "sex AND idnum1",
                syntactically_valid=True,
                valid=True,
                ptinfo_satisfies_id_policy=True,
                test_critical_single_numerical_id=True,
                critical_single_numerical_id=1,
                compatible_with_tablet_id_policy=True,
                is_idnum_mandatory_in_policy={1: True, 3: False}
            ),
            TestRig(
                "sex AND NOT idnum1",
                syntactically_valid=True,
                valid=False,  # not compatible with tablet policy
                ptinfo_satisfies_id_policy=False,
                test_critical_single_numerical_id=True,
                critical_single_numerical_id=None,
                compatible_with_tablet_id_policy=False,
                is_idnum_mandatory_in_policy={1: False, 3: False}
            ),
            TestRig(
                "sex AND NOT idnum2",
                syntactically_valid=True,
                valid=False,  # not compatible with tablet policy
                test_critical_single_numerical_id=True,
                critical_single_numerical_id=None,
                compatible_with_tablet_id_policy=False,
                is_idnum_mandatory_in_policy={1: False, 3: False}
            ),
            TestRig(
                "sex AND NOT idnum1 AND idnum3",
                syntactically_valid=True,
                valid=True,
                test_critical_single_numerical_id=True,
                critical_single_numerical_id=3,
                compatible_with_tablet_id_policy=True,
                is_idnum_mandatory_in_policy={1: False, 3: True}
            ),
            TestRig(
                "sex AND NOT idnum2 AND idnum10",
                syntactically_valid=True,
                valid=True,
                test_critical_single_numerical_id=True,
                critical_single_numerical_id=10,
                compatible_with_tablet_id_policy=True,
                is_idnum_mandatory_in_policy={1: False, 3: False}
            ),
            TestRig(
                "sex AND NOT idnum1 AND NOT idnum2 AND NOT idnum3",
                syntactically_valid=True,
                valid=False,  # not compatible with tablet policy
                test_critical_single_numerical_id=True,
                critical_single_numerical_id=None,
                compatible_with_tablet_id_policy=False,
                is_idnum_mandatory_in_policy={1: False, 3: False}
            ),
            TestRig(
                "sex AND NOT (idnum1 OR idnum2 OR idnum3)",  # same as previous
                syntactically_valid=True,
                valid=False,  # not compatible with tablet policy
                test_critical_single_numerical_id=True,
                critical_single_numerical_id=None,
                compatible_with_tablet_id_policy=False,
                is_idnum_mandatory_in_policy={1: False, 3: False}
            ),
            TestRig(
                "NOT (sex OR forename OR surname OR dob OR anyidnum)",
                syntactically_valid=True,
                valid=False,  # not compatible with tablet policy
                test_critical_single_numerical_id=True,
                critical_single_numerical_id=None,
                compatible_with_tablet_id_policy=False,
                is_idnum_mandatory_in_policy={1: False, 3: False}
            ),
            TestRig(
                """
                    NOT (sex OR forename OR surname OR dob OR
                         idnum1 OR idnum2 OR idnum3)
                """,
                syntactically_valid=True,
                valid=False,  # not compatible with tablet policy
                test_critical_single_numerical_id=True,
                critical_single_numerical_id=None,
                compatible_with_tablet_id_policy=False,
                is_idnum_mandatory_in_policy={1: False, 3: False}
            ),
            TestRig(
                "sex AND idnum1 AND otheridnum AND NOT address",
                syntactically_valid=True,
                valid=True,
                test_critical_single_numerical_id=True,
                critical_single_numerical_id=None,
                compatible_with_tablet_id_policy=True,
                is_idnum_mandatory_in_policy={1: True, 3: False}
            ),
            TestRig(
                "sex AND idnum1 AND NOT (otheridnum OR forename OR surname OR "
                "dob OR address OR gp OR otherdetails)",
                syntactically_valid=True,
                valid=True,
                test_critical_single_numerical_id=True,
                critical_single_numerical_id=1,
                compatible_with_tablet_id_policy=True,
                is_idnum_mandatory_in_policy={1: True, 3: False}
            ),
            TestRig(
                "forename AND surname AND dob AND sex AND anyidnum",
                syntactically_valid=True,
                valid=True,
                test_critical_single_numerical_id=True,
                critical_single_numerical_id=None,
                compatible_with_tablet_id_policy=True,
                is_idnum_mandatory_in_policy={1: False, 3: False}
            ),
            TestRig(
                "forename AND surname AND email AND sex AND idnum1",
                syntactically_valid=True,
                valid=True,
                test_critical_single_numerical_id=True,
                critical_single_numerical_id=1,
                compatible_with_tablet_id_policy=True,
                is_idnum_mandatory_in_policy={1: True, 3: False}
            ),
            TestRig(
                "email AND sex AND idnum1",
                syntactically_valid=True,
                valid=True,
                ptinfo_satisfies_id_policy=True,
                test_critical_single_numerical_id=True,
                critical_single_numerical_id=1,
                compatible_with_tablet_id_policy=True,
                is_idnum_mandatory_in_policy={1: True, 3: False}
            ),
        ]

        test_idnums = [None, -1, 1, 3]
        correct_msg = "... correct"

        for tp in test_policies:
            policy_string = tp.policy
            log.warning("Testing {!r}", policy_string)
            p = TokenizedPolicy(policy_string)
            p.set_valid_idnums(valid_idnums)

            log.debug("Testing is_syntactically_valid()")
            x = p.is_syntactically_valid()
            self.assertIsInstance(x, bool)
            log.info("is_syntactically_valid() -> {!r}".format(x))
            if tp.syntactically_valid is not None:
                self.assertEqual(x, tp.syntactically_valid)
                log.info(correct_msg)

            log.debug("Testing is_valid()")
            x = p.is_valid(verbose=True)
            self.assertIsInstance(x, bool)
            log.info("is_valid(valid_idnums={!r}) -> {!r}".format(
                valid_idnums, x))
            if tp.valid is not None:
                self.assertEqual(x, tp.valid)
                log.info(correct_msg)

            log.debug("Testing is_idnum_mandatory_in_policy()")
            for which_idnum in test_idnums:
                log.debug("... for which_idnum = {}", which_idnum)
                x = p.is_idnum_mandatory_in_policy(
                    which_idnum=which_idnum,
                    valid_idnums=valid_idnums,
                    verbose=True
                )
                self.assertIsInstance(x, bool)
                log.info(
                    "is_idnum_mandatory_in_policy(which_idnum={!r}, "
                    "valid_idnums={!r}) -> {!r}".format(
                        which_idnum, valid_idnums, x))
                if tp.is_idnum_mandatory_in_policy:
                    if which_idnum in tp.is_idnum_mandatory_in_policy:
                        self.assertEqual(
                            x, tp.is_idnum_mandatory_in_policy[which_idnum])
                        log.info(correct_msg)

            log.debug("Testing find_critical_single_numerical_id()")
            x = p.find_critical_single_numerical_id(valid_idnums=valid_idnums,
                                                    verbose=True)
            self.assertIsInstanceOrNone(x, int)
            log.info("find_critical_single_numerical_id(valid_idnums={!r}) "
                     "-> {!r}".format(valid_idnums, x))
            if tp.test_critical_single_numerical_id:
                self.assertEqual(x, tp.critical_single_numerical_id)
                log.info(correct_msg)

            log.debug("Testing compatible_with_tablet_id_policy()")
            x = p.compatible_with_tablet_id_policy(valid_idnums=valid_idnums,
                                                   verbose=True)
            self.assertIsInstance(x, bool)
            log.info("compatible_with_tablet_id_policy(valid_idnums={!r}) "
                     "-> {!r}".format(valid_idnums, x))
            if tp.compatible_with_tablet_id_policy is not None:
                self.assertEqual(x, tp.compatible_with_tablet_id_policy)
                log.info(correct_msg)

            log.debug("Testing satisfies_id_policy()")
            x = p.satisfies_id_policy(bpi)
            self.assertIsInstance(x, bool)
            log.info("satisfies_id_policy(bpi={}) -> {!r}".format(bpi, x))
            if tp.ptinfo_satisfies_id_policy is not None:
                self.assertEqual(x, tp.ptinfo_satisfies_id_policy)
                log.info(correct_msg)
