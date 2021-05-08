#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_email.py

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

**Email functions/log class.**

"""

import email.utils
import logging
from typing import List, Sequence, Tuple

from cardinal_pythonlib.datetimefunc import (
    convert_datetime_to_utc,
    get_now_localtz_pendulum,
    get_now_utc_pendulum,
)
from cardinal_pythonlib.email.sendmail import (
    COMMASPACE,
    make_email,
    send_msg,
    STANDARD_SMTP_PORT,
    STANDARD_TLS_PORT,
)
from cardinal_pythonlib.logs import BraceStyleAdapter
from sqlalchemy.orm import reconstructor
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import (
    Boolean,
    BigInteger,
    DateTime,
    Integer,
    Text,
)

from camcops_server.cc_modules.cc_sqlalchemy import Base
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CharsetColType,
    EmailAddressColType,
    HostnameColType,
    LongText,
    MimeTypeColType,
    Rfc2822DateColType,
    UserNameExternalColType,
)

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Email class
# =============================================================================

class Email(Base):
    """
    Class representing an e-mail sent from CamCOPS.

    This is abstract, in that it doesn't care about the purpose of the e-mail.
    It's cross-referenced from classes that use it, such as
    :class:`camcops_server.cc_modules.cc_exportmodels.ExportedTaskEmail`.
    """
    __tablename__ = "_emails"

    # -------------------------------------------------------------------------
    # Basic things
    # -------------------------------------------------------------------------
    id = Column(
        "id", BigInteger, primary_key=True, autoincrement=True,
        comment="Arbitrary primary key"
    )
    created_at_utc = Column(
        "created_at_utc", DateTime,
        comment="Date/time message was created (UTC)"
    )
    # -------------------------------------------------------------------------
    # Headers
    # -------------------------------------------------------------------------
    date = Column(
        "date", Rfc2822DateColType,
        comment="Email date in RFC 2822 format"
    )
    from_addr = Column(
        "from_addr", EmailAddressColType,
        comment="Email 'From:' field"
    )
    sender = Column(
        "sender", EmailAddressColType,
        comment="Email 'Sender:' field"
    )
    reply_to = Column(
        "reply_to", EmailAddressColType,
        comment="Email 'Reply-To:' field"
    )
    to = Column(
        "to", Text,
        comment="Email 'To:' field"
    )
    cc = Column(
        "cc", Text,
        comment="Email 'Cc:' field"
    )
    bcc = Column(
        "bcc", Text,
        comment="Email 'Bcc:' field"
    )
    subject = Column(
        "subject", Text,
        comment="Email 'Subject:' field"
    )
    # -------------------------------------------------------------------------
    # Body, message
    # -------------------------------------------------------------------------
    body = Column(
        "body", Text,
        comment="Email body"
    )
    content_type = Column(
        "content_type", MimeTypeColType,
        comment="MIME type for e-mail body"
    )
    charset = Column(
        "charset", CharsetColType,
        comment="Character set for e-mail body"
    )
    msg_string = Column(
        "msg_string", LongText,
        comment="Full encoded e-mail"
    )
    # -------------------------------------------------------------------------
    # Server
    # -------------------------------------------------------------------------
    host = Column(
        "host", HostnameColType,
        comment="Email server"
    )
    port = Column(
        "port", Integer,
        comment="Port number on e-mail server"
    )
    username = Column(
        "username", UserNameExternalColType,
        comment="Username on e-mail server"
    )
    use_tls = Column(
        "use_tls", Boolean,
        comment="Use TLS?"
    )
    # -------------------------------------------------------------------------
    # Status
    # -------------------------------------------------------------------------
    sent = Column(
        "sent", Boolean, default=False, nullable=False,
        comment="Sent?"
    )
    sent_at_utc = Column(
        "sent_at_utc", DateTime,
        comment="Date/time message was sent (UTC)"
    )
    sending_failure_reason = Column(
        "sending_failure_reason", Text,
        comment="Reason for sending failure"
    )

    def __init__(self,
                 from_addr: str = "",
                 date: str = None,
                 sender: str = "",
                 reply_to: str = "",
                 to: str = "",
                 cc: str = "",
                 bcc: str = "",
                 subject: str = "",
                 body: str = "",
                 content_type: str = "text/plain",
                 charset: str = "utf8",
                 attachment_filenames: Sequence[str] = None,
                 attachments_binary: Sequence[Tuple[str, bytes]] = None,
                 save_msg_string: bool = False) -> None:
        """
        Args:
            from_addr: name of the sender for the "From:" field
            date: e-mail date in RFC 2822 format, or ``None`` for "now"
            sender: name of the sender for the "Sender:" field
            reply_to: name of the sender for the "Reply-To:" field

            to: e-mail address(es) of the recipients for "To:" field, as a
                CSV list
            cc: e-mail address(es) of the recipients for "Cc:" field, as a
                CSV list
            bcc: e-mail address(es) of the recipients for "Bcc:" field, as a
                CSV list

            subject: e-mail subject
            body: e-mail body
            content_type: MIME type for body content, default ``text/plain``
            charset: character set for body; default ``utf8``
            charset:

            attachment_filenames: filenames of attachments to add
            attachments_binary: binary attachments to add, as a list of
                ``filename, bytes`` tuples

            save_msg_string: save the encoded message string? (May take
                significant space in the database).
        """
        # Note: we permit from_addr to be blank only for automated database
        # copying.

        # ---------------------------------------------------------------------
        # Timestamp
        # ---------------------------------------------------------------------
        now_local = get_now_localtz_pendulum()
        self.created_at_utc = convert_datetime_to_utc(now_local)

        # -------------------------------------------------------------------------
        # Arguments
        # -------------------------------------------------------------------------
        if not date:
            date = email.utils.format_datetime(now_local)
        attachment_filenames = attachment_filenames or []  # type: List[str]
        attachments_binary = attachments_binary or []  # type: List[Tuple[str, bytes]]  # noqa
        if attachments_binary:
            attachment_binary_filenames, attachment_binaries = zip(
                *attachments_binary)
        else:
            attachment_binary_filenames = []  # type: List[str]
            attachment_binaries = []  # type: List[bytes]
        # ... https://stackoverflow.com/questions/13635032/what-is-the-inverse-function-of-zip-in-python  # noqa
        # Other checks performed by our e-mail function below

        # ---------------------------------------------------------------------
        # Transient fields
        # ---------------------------------------------------------------------
        self.password = None
        self.msg = make_email(
            from_addr=from_addr,
            date=date,
            sender=sender,
            reply_to=reply_to,
            to=to,
            cc=cc,
            bcc=bcc,
            subject=subject,
            body=body,
            content_type=content_type,
            attachment_filenames=attachment_filenames,
            attachment_binaries=attachment_binaries,
            attachment_binary_filenames=attachment_binary_filenames,
        ) if from_addr else None

        # ---------------------------------------------------------------------
        # Database fields
        # ---------------------------------------------------------------------
        self.date = date
        self.from_addr = from_addr
        self.sender = sender
        self.reply_to = reply_to
        self.to = to
        self.cc = cc
        self.bcc = bcc
        self.subject = subject
        self.body = body
        self.content_type = content_type
        self.charset = charset
        if save_msg_string:
            self.msg_string = self.msg.as_string()

    @reconstructor
    def init_on_load(self) -> None:
        """
        Called when SQLAlchemy recreates an object; see
        https://docs.sqlalchemy.org/en/latest/orm/constructors.html.
        """
        self.password = None
        self.msg = None

    def send(self,
             host: str,
             username: str,
             password: str,
             port: int = None,
             use_tls: bool = True) -> bool:
        """
        Sends message and returns success.
        """
        if port is None:
            port = STANDARD_TLS_PORT if use_tls else STANDARD_SMTP_PORT

        msg = None
        msg_string = None
        if self.msg:
            msg = self.msg
        elif self.msg_string:
            msg_string = self.msg_string
        else:
            log.error("Can't send message; not present (not saved?)")
            return False

        # Password not always required (for insecure servers...)

        if self.sent:
            log.info("Resending message")

        self.host = host
        self.port = port
        self.username = username
        # don't save password
        self.use_tls = use_tls
        to_addrs = COMMASPACE.join(
            x for x in [self.to, self.cc, self.bcc] if x)
        header_components = filter(None, [
            f"To: {self.to}" if self.to else "",
            f"Cc: {self.cc}" if self.cc else "",
            f"Bcc: {self.bcc}" if self.bcc else "",  # noqa
            f"Subject: {self.subject}" if self.subject else "",
        ])
        log.info("Sending email -- {}", " -- ".join(header_components))
        try:
            send_msg(
                from_addr=self.from_addr,
                to_addrs=to_addrs,
                host=host,
                user=username,
                password=password,
                port=port,
                use_tls=use_tls,
                msg=msg,
                msg_string=msg_string,
            )
            log.debug("... sent")
            self.sent = True
            self.sent_at_utc = get_now_utc_pendulum()
            self.sending_failure_reason = None
        except RuntimeError as e:
            log.error("Failed to send e-mail: {!s}", e)
            if not self.sent:
                self.sent = False
                self.sending_failure_reason = str(e)
