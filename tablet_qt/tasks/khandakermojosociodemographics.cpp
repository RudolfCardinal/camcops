/*
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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.
*/

#include "khandakermojosociodemographics.h"
#include "common/cssconst.h"
#include "common/textconst.h"
#include "lib/convert.h"
#include "lib/uifunc.h"
#include "lib/version.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qupage.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"


const QString KhandakerMojoSociodemographics::KHANDAKER2MOJOSOCIODEMOGRAPHICS_TABLENAME(
    "khandaker_mojo_sociodemographics");

const QString Q_XML_PREFIX = "q_";

struct KhandakerMojoSocQInfo {
    KhandakerMojoSocQInfo(const QString& stem, const int max_option,
           const bool has_other = false) :
        m_fieldname(stem),
        m_max_option(max_option),
        m_has_other(has_other)
    {
        if (has_other) {
            m_other_fieldname = "other_" + stem;
        }
        m_question_xml_name = Q_XML_PREFIX + stem;
    }

    bool hasOther() const { return m_has_other; }
    QString getFieldname() const { return m_fieldname; }
    QString getOtherFieldname() const { return m_other_fieldname; }
    QString getQuestionXmlName() const { return m_question_xml_name; }
    int getMaxOption() const { return m_max_option; }

private:
    QString m_fieldname;
    QString m_other_fieldname;
    QString m_question_xml_name;
    int m_max_option;
    bool m_has_other;
};
using KQInfo = KhandakerMojoSocQInfo;

const QVector<KQInfo> MC_QUESTIONS{
    KQInfo("gender", 2, true),
    KQInfo("ethnicity", 10, true),
    KQInfo("with_whom_live", 7, true),
    KQInfo("relationship_status", 4),
    KQInfo("education", 4),
    KQInfo("employment", 7, true),
    KQInfo("accommodation", 6, true),
};


void initializeKhandakerMojoSociodemographics(TaskFactory& factory)
{
    static TaskRegistrar<KhandakerMojoSociodemographics> registered(factory);
}


KhandakerMojoSociodemographics::KhandakerMojoSociodemographics(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, KHANDAKER2MOJOSOCIODEMOGRAPHICS_TABLENAME,
         false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    for (const auto& info : MC_QUESTIONS) {
        addField(info.getFieldname(), QVariant::Int);

        if (info.hasOther()) {
            addField(info.getOtherFieldname(), QVariant::String);
        }
    }

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}



// ============================================================================
// Class info
// ============================================================================

QString KhandakerMojoSociodemographics::shortname() const
{
    return "Khandaker_MOJO_Sociodemographics";
}


QString KhandakerMojoSociodemographics::longname() const
{
    return tr("Khandaker GM — MOJO — Sociodemographics");
}


QString KhandakerMojoSociodemographics::description() const
{
    return tr("Sociodemographics questionnaire for MOJO study.");
}




// ============================================================================
// Instance info
// ============================================================================

bool KhandakerMojoSociodemographics::isComplete() const
{
    for (const auto& info : MC_QUESTIONS) {
        if (valueIsNull(info.getFieldname())) {
            return false;
        }

        if (answeredOther(info) && valueIsNull(info.getOtherFieldname())) {
            return false;
        }
    }

    return true;
}


QStringList KhandakerMojoSociodemographics::summary() const
{
    return QStringList{TextConst::noSummarySeeFacsimile()};
}


QStringList KhandakerMojoSociodemographics::detail() const
{
    QStringList lines;

    for (const auto& info : MC_QUESTIONS) {
        lines.append(xstring(info.getQuestionXmlName()));
        lines.append(QString("<b>%1</b>").arg(getAnswerText(info)));
    }

    return completenessInfo() + lines;
}


OpenableWidget* KhandakerMojoSociodemographics::editor(const bool read_only)
{
    QuPagePtr page(new QuPage);
    page->setTitle(description());
    page->addElement(new QuHeading(xstring("title")));

    for (const KQInfo& info : MC_QUESTIONS) {
        page->addElement(
            (new QuText(xstring(info.getQuestionXmlName())))->setBold());

        FieldRefPtr fieldref = fieldRef(info.getFieldname());
        connect(fieldref.data(), &FieldRef::valueChanged,
                this, &KhandakerMojoSociodemographics::updateMandatory);

        QuMcq* mcq = new QuMcq(fieldref, getOptions(info));
        mcq->setHorizontal(true);
        mcq->setAsTextButton(true);
        page->addElement(mcq);

        if (info.hasOther()) {
            QString fieldname = info.getOtherFieldname();
            auto text_edit = new QuTextEdit(fieldRef(fieldname));
            text_edit->addTag(fieldname);
            page->addElement(text_edit);
        }

        page->addElement(new QuSpacer(QSize(uiconst::BIGSPACE,
                                            uiconst::BIGSPACE)));
    }

    QVector<QuPagePtr> pages{page};

    m_questionnaire = new Questionnaire(m_app, pages);
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);

    updateMandatory();

    return m_questionnaire;
}


bool KhandakerMojoSociodemographics::answeredOther(
        const KhandakerMojoSocQInfo& info) const
{
    // For this task, the 'other' option is always the last one
    const int answer = valueInt(info.getFieldname());

    return info.hasOther() && answer == info.getMaxOption();
}


NameValueOptions KhandakerMojoSociodemographics::getOptions(
    const KhandakerMojoSocQInfo& info) const
{
    NameValueOptions options;

    for (int i = 0; i <= info.getMaxOption(); i++) {
        const QString name = getOptionName(info, i);
        options.append(NameValuePair(name, i));
    }

    return options;
}

QString KhandakerMojoSociodemographics::getOptionName(
    const KhandakerMojoSocQInfo& info, const int index) const
{
    return xstring(QString("%1_option%2").arg(info.getFieldname()).arg(index));
}


QString KhandakerMojoSociodemographics::getAnswerText(
    const KhandakerMojoSocQInfo& info) const
{
    if (valueIsNull(info.getFieldname())) {
        return convert::NULL_STR;
    }

    const int answer_value = valueInt(info.getFieldname());

    QString answer_text = getOptionName(info, answer_value);

    if (answeredOther(info)) {
        answer_text = QString("%1 (%2)").arg(
            answer_text,
            prettyValue(info.getOtherFieldname())
        );
    }

    return QString("%1 — %2").arg(answer_value).arg(answer_text);
}

// ============================================================================
// Signal handlers
// ============================================================================

void KhandakerMojoSociodemographics::updateMandatory()
{
    // This could be more efficient with lots of signal handlers, but...

    for (const auto& info : MC_QUESTIONS) {
        if (info.hasOther()) {
            const bool mandatory = answeredOther(info);
            fieldRef(info.getOtherFieldname())->setMandatory(mandatory);

            if (m_questionnaire) {
                m_questionnaire->setVisibleByTag(info.getOtherFieldname(),
                                                 mandatory);
            }
        }
    }
}
