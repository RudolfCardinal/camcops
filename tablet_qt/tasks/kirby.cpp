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

// #define DEBUG_SHOW_K
// #define DEBUG_KIRBY_CALCS
// #define DEBUG_WILEYTO_CALCS

#include "kirby.h"
#include <QtGlobal>  // for qMax()
#include "../taskxtra/kirbyrewardpair.h"
#include "../taskxtra/kirbytrial.h"
#include "common/textconst.h"
#include "db/ancillaryfunc.h"
#include "lib/convert.h"
#include "lib/version.h"
#include "maths/eigenfunc.h"
#include "maths/include_eigen_core.h"
#include "maths/logisticregression.h"
#include "maths/mathfunc.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"


// ============================================================================
// Constants
// ============================================================================

const QString Kirby::KIRBY_TABLENAME("kirby_mcq");


// ============================================================================
// Factory function
// ============================================================================

void initializeKirby(TaskFactory& factory)
{
    static TaskRegistrar<Kirby> registered(factory);
}


// ============================================================================
// Standard sequence
// ============================================================================

const QVector<KirbyRewardPair> TRIALS{
    {54, 55, 117},  // e.g. "Would you prefer £54 now, or £55 in 117 days?"
    {55, 75, 61},
    {19, 25, 53},
    {31, 85, 7},
    {14, 25, 19},

    {47, 50, 160},
    {15, 35, 13},
    {25, 60, 14},
    {78, 80, 162},
    {40, 55, 62},

    {11, 30, 7},
    {67, 75, 119},
    {34, 35, 186},
    {27, 50, 21},
    {69, 85, 91},

    {49, 60, 89},
    {80, 85, 157},
    {24, 35, 29},
    {33, 80, 14},
    {28, 30, 179},

    {34, 50, 30},
    {25, 30, 80},
    {41, 75, 20},
    {54, 60, 111},
    {54, 80, 30},

    {22, 25, 136},
    {20, 55, 7},
};
const int TOTAL_N_TRIALS = TRIALS.size();  // 27


// ============================================================================
// Main class: constructor
// ============================================================================

Kirby::Kirby(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, KIRBY_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    // No fields beyond the basics.

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Kirby::shortname() const
{
    return "KirbyMCQ";
}


QString Kirby::longname() const
{
    return tr("Kirby et al. 1999 Monetary Choice Questionnaire");
}


QString Kirby::description() const
{
    return tr("Series of hypothetical choices to measure delay discounting.");
}


Version Kirby::minimumServerVersion() const
{
    return Version(2, 3, 3);
}


// ============================================================================
// Ancillary management
// ============================================================================

QStringList Kirby::ancillaryTables() const
{
    return QStringList{KirbyTrial::KIRBY_TRIAL_TABLENAME};
}


QString Kirby::ancillaryTableFKToTaskFieldname() const
{
    return KirbyTrial::FN_FK_TO_TASK;
}


void Kirby::loadAllAncillary(const int pk)
{
    const OrderBy order_by{{KirbyTrial::FN_TRIAL, true}};
    ancillaryfunc::loadAncillary<KirbyTrial, KirbyTrialPtr>(
                m_trials, m_app, m_db,
                KirbyTrial::FN_FK_TO_TASK, order_by, pk);
}


QVector<DatabaseObjectPtr> Kirby::getAncillarySpecimens() const
{
    return QVector<DatabaseObjectPtr>{
        KirbyTrialPtr(new KirbyTrial(m_app, m_db)),
    };
}


QVector<DatabaseObjectPtr> Kirby::getAllAncillary() const
{
    QVector<DatabaseObjectPtr> ancillaries;
    for (const KirbyTrialPtr& trial : m_trials) {
        ancillaries.append(trial);
    }
    return ancillaries;
}


// ============================================================================
// Instance info
// ============================================================================

bool Kirby::isComplete() const
{
    if (m_trials.length() != TOTAL_N_TRIALS) {
        return false;
    }
    for (auto t : m_trials) {
        if (!t->answered()) {
            return false;
        }
    }
    return true;
}


QStringList Kirby::summary() const
{
    const QVector<KirbyRewardPair> results = allChoiceResults();
    const double k_kirby = kKirby(results);
    const double k_wileyto = kWileyto(results);
    const int dp = 6;
    return QStringList({
        tr("<i>k</i> (days<sup>–1</sup>, Kirby 2000 method): <b>%1</b> "
           "(decay to half value at <b>%2</b> days).").arg(
                           convert::toDp(k_kirby, dp),
                           convert::toDp(1 / k_kirby, 0)),
        tr("<i>k</i> (days<sup>–1</sup>, Wileyto 2004 method): <b>%1</b> "
           "(decay to half value at <b>%2</b> days).").arg(
                           convert::toDp(k_wileyto, dp),
                           convert::toDp(1 / k_wileyto, 0)),
    });
}


QStringList Kirby::detail() const
{
    QStringList choices;
    const int dp = 6;
    const QVector<KirbyRewardPair> results = allTrialResults();
    for (int i = 0; i < results.size(); ++i) {
        const KirbyRewardPair& pair = results.at(i);
        const int trial_num = i + 1;
        choices.append(
            QString("%1. %2 <i>(k<sub>indiff</sub> = %3)</i> <b>%4</b>").arg(
                        QString::number(trial_num),
                        pair.question(),
                        convert::toDp(pair.kIndifference(), dp),
                        pair.answer()));
    }
    choices.append("");
    return choices + summary();
}


OpenableWidget* Kirby::editor(const bool read_only)
{
    // There are a few ways of doing this, but the questionnaire way is
    // perfectly reasonable.

    QVector<QuPage*> pages;

    // Intro
    QuPage* intro_page = new QuPage();
    intro_page->setTitle(xstring("intro_title"));
    intro_page->addElement(new QuText(xstring("intro")));
    pages.append(intro_page);

    // Trials
    for (int trial_num = 1; trial_num <= TOTAL_N_TRIALS; ++ trial_num) {
        QuPage* p = new QuPage();
        p->setTitle(QString("%1 %2").arg(textconst.question(),
                                         QString::number(trial_num)));
        KirbyTrialPtr trial = getTrial(trial_num);  // may create it
        const KirbyRewardPair choice = trial->info();
        FieldRef::GetterFunction getterfunc = std::bind(
                    &Kirby::getChoice, this, trial_num);
        FieldRef::SetterFunction setterfunc = std::bind(
                    &Kirby::choose, this, trial_num, std::placeholders::_1);
        FieldRefPtr fieldref(new FieldRef(getterfunc, setterfunc, true));
        NameValueOptions options{
            {choice.sirString(), false},
            {choice.ldrString(), true},  // the boolean value is "chose LDR"
        };
        p->addElement(new QuMcq(fieldref, options));
#ifdef DEBUG_SHOW_K
        QString explanation = QString(
                    "Indifference k: %1. A subject with a higher k (more "
                    "impulsive) will choose the small immediate reward. A "
                    "subject with a lower k (less impulsive) will choose the "
                    "large delayed reward.").arg(choice.kIndifference());
        QuText* explan_e = new QuText(explanation);
        explan_e->setItalic();
        p->addElement(explan_e);
#endif
        pages.append(p);
    }

    // Thanks
    QuPage* thanks_page = new QuPage();
    thanks_page->setTitle(xstring("thanks_title"));
    thanks_page->addElement(new QuText(xstring("thanks")));
    pages.append(thanks_page);

    m_questionnaire = new Questionnaire(m_app, pages);
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);
    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

KirbyTrialPtr Kirby::getTrial(const int trial_num)
{
    Q_ASSERT(trial_num >= 1 && trial_num <= TOTAL_N_TRIALS);
    for (auto t : m_trials) {
        if (t->trialNum() == trial_num) {
            return t;
        }
    }
    // None found; create new.
    const int trial_num_zb = trial_num - 1;  // zero-based
    const KirbyRewardPair& choice = TRIALS[trial_num_zb];
    KirbyTrialPtr t = KirbyTrialPtr(new KirbyTrial(
        pkvalueInt(), trial_num, choice, m_app, m_db));  // will save
    m_trials.append(t);
    sortTrials();  // Re-sort (this shouldn't be necessary, but...)
    return t;
}


void Kirby::sortTrials()
{
    std::sort(
        m_trials.begin(), m_trials.end(),
        [](const KirbyTrialPtr& a, const KirbyTrialPtr& b) {
            // lambda comparator
            return a->trialNum() > b->trialNum();
        }
    );
}


QVector<KirbyRewardPair> Kirby::allTrialResults() const
{
    QVector<KirbyRewardPair> v;
    for (auto t : m_trials) {
        v.append(t->info());
    }
    return v;
}


QVector<KirbyRewardPair> Kirby::allChoiceResults() const
{
    QVector<KirbyRewardPair> v;
    for (auto t : m_trials) {
        if (t->answered()) {
            v.append(t->info());
        }
    }
    return v;
}


double Kirby::kKirby(const QVector<KirbyRewardPair>& results)
{
    // 1. For every k value assessed by the questions, establish the degree
    //    of consistency.
    QMap<double, int> consistency;  // maps k to n_consistent_choices
    for (const KirbyRewardPair& pair : results) {
        const double k = pair.kIndifference();
        if (!consistency.contains(k)) {
            consistency[k] = nChoicesConsistent(k, results);
        }
    }
    // 2. Restrict to the results that are equally and maximally consistent.
    QList<int> consistency_values = consistency.values();
    const int max_consistency = *std::max_element(consistency_values.begin(),
                                                  consistency_values.end());
    QVector<double> good_k_values;
    QMap<double, int>::const_iterator it = consistency.constBegin();
    while (it != consistency.constEnd()) {
        if (it.value() == max_consistency) {
            good_k_values.append(it.key());
        }
        ++it;
    }
    // 3. Take the geometric mean of those good k values.
    double subject_k = mathfunc::geometricMean(good_k_values);
#ifdef DEBUG_KIRBY_CALCS
    qDebug().nospace()
            << "consistency = " << consistency
            << ", consistency_values = " << consistency_values
            << ", max_consistency = " << max_consistency
            << ", good_k_values = " << good_k_values
            << ", subject_k = " << subject_k;
#endif
    return subject_k;
}


int Kirby::nChoicesConsistent(const double k,
                              const QVector<KirbyRewardPair>& results)
{
    int n_consistent = 0;
    for (const KirbyRewardPair& pair : results) {
        if (pair.choiceConsistent(k)) {
            ++n_consistent;
        }
    }
    return n_consistent;
}


double Kirby::kWileyto(const QVector<KirbyRewardPair>& results)
{
    const int n_predictors = 2;
    const int n_observations = results.length();
    if (n_observations == 0) {
        return std::numeric_limits<double>::quiet_NaN();
        // ... or it will crash when we try to operate with empty Eigen objects
    }
    Eigen::MatrixXd X(n_observations, n_predictors);
    Eigen::VectorXi y(n_observations);
    for (int i = 0; i < n_observations; ++i) {
        const KirbyRewardPair& pair = results.at(i);
        const double a1 = pair.sir;
        const double a2 = pair.ldr;
        const double d2 = pair.delay_days;
        const double predictor1 = 1 - (a2 / a1);
        const double predictor2 = d2;
        X(i, 0) = predictor1;
        X(i, 1) = predictor2;
        y(i) = pair.chose_ldr.toInt();  // bool to int
    }
    LogisticRegression lr;
    lr.fitDirectly(X, y);
    const Eigen::VectorXd coeffs = lr.coefficients();
    const double beta1 = coeffs(0);
    const double beta2 = coeffs(1);
    const double k = beta2 / beta1;
#ifdef DEBUG_WILEYTO_CALCS
    // Since qStringFromEigenMatrixOrArray is a template function, it's hard to
    // alias and this doesn't work:
    // constexpr auto qs = eigenfunc::qStringFromEigenMatrixOrArray;
#define QS eigenfunc::qStringFromEigenMatrixOrArray
    qDebug().nospace().noquote()
            << "Wileyto: y = " << QS(y)
            << ", X = " << QS(X)
            << ", coeffs = " << QS(coeffs)
            << ", predicted probabilities = " << QS(lr.predictProb());
#endif
    return k;
}


// ============================================================================
// Questionnaire callbacks
// ============================================================================

QVariant Kirby::getChoice(const int trial_num)
{
    KirbyTrialPtr trial = getTrial(trial_num);
    return trial->getChoice();
}


bool Kirby::choose(const int trial_num, const QVariant& chose_ldr)
{
    KirbyTrialPtr trial = getTrial(trial_num);
    trial->recordChoice(chose_ldr.toBool());
    return true;
}


// ============================================================================
// Text constants
// ============================================================================
// ... so they appear in Qt Linguist within the right class, without having to
// make KirbyRewardPair a QObject.

QString Kirby::textXtoday()
{
    return tr("%1 today");
}

QString Kirby::textXinYdays()
{
    return tr("%1 in %2 days");
}


QString Kirby::textWouldYouPreferXOrY()
{
    return tr("Would you prefer %1, or %2?");
}
