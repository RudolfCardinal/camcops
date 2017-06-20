/*
    Copyright (C) 2012-2017 Rudolf Cardinal (rudolf@pobox.com).

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

#define DEBUG_OPTIONS

#include "testmenu.h"
#include <QCoreApplication>
#include <QMediaPlayer>
#include <QProgressDialog>
#include <QThread>
#include "common/platform.h"
#include "diagnosis/icd10.h"
#include "diagnosis/icd9cm.h"
#include "lib/convert.h"
#include "lib/filefunc.h"
#include "lib/networkmanager.h"
#include "lib/uifunc.h"
#include "lib/slownonguifunctioncaller.h"
#include "lib/soundfunc.h"
#include "maths/ccrandom.h"
#include "maths/eigenfunc.h"
#include "maths/logisticdescriptives.h"
#include "maths/logisticregression.h"
#include "maths/mathfunc.h"
#include "menulib/menuitem.h"
#include "menu/widgettestmenu.h"
#include "tasklib/taskfactory.h"  // for TaskPtr
#include "tasks/phq9.h"

const int EXPENSIVE_FUNCTION_DURATION_MS = 10000;


TestMenu::TestMenu(CamcopsApp& app)
    : MenuWindow(app, tr("CamCOPS self-tests"),
                 uifunc::iconFilename(uiconst::CBS_SPANNER)),
      m_player(nullptr)
{
    m_items = {
        MenuItem(tr("User testing")).setLabelOnly(),
        MenuItem(
            tr("Test sound"),
            std::bind(&TestMenu::testSound, this)
        ).setNotIfLocked(),
        MenuItem(tr("Developer testing")).setLabelOnly(),
        MenuItem(
            tr("Test debug console"),
            std::bind(&TestMenu::testDebugConsole, this)
        ),
        MenuItem(
            tr("Test network (HTTP)"),
            std::bind(&TestMenu::testHttp, this)
        ).setNotIfLocked(),
        MenuItem(
            tr("Test network (HTTPS/SSL)"),
            std::bind(&TestMenu::testHttps, this)
        ).setNotIfLocked(),
#ifdef DEBUG_OPTIONS
        MenuItem(
            tr("Test PHQ9 creation (nothing is saved)"),
            std::bind(&TestMenu::testPhq9Creation, this)
        ).setNotIfLocked(),
        MenuItem(
            tr("Test ICD-10 code set creation (nothing is saved)"),
            std::bind(&TestMenu::testIcd10CodeSetCreation, this)
        ),
        MenuItem(
            tr("Test ICD-9-CM code set creation (nothing is saved)"),
            std::bind(&TestMenu::testIcd9cmCodeSetCreation, this)
        ),
#endif
        MenuItem(
            tr("Test HTML display, and fullscreen display"),
            HtmlMenuItem("Example HTML: this window should be full-screen",
                         filefunc::taskHtmlFilename("ace3"),
                         "", true)
        ),
        MenuItem(
            tr("Test progress dialog"),
            std::bind(&TestMenu::testProgress, this)
        ),
        MenuItem(
            tr("Test wait dialog"),
            std::bind(&TestMenu::testWait, this)
        ),
        MenuItem(
            tr("Test size formatter"),
            std::bind(&TestMenu::testSizeFormatter, this)
        ),
        MenuItem(
            tr("Test conversions"),
            std::bind(&TestMenu::testConversions, this)
        ),
        MenuItem(
            tr("Test Eigen functions"),
            std::bind(&TestMenu::testEigenFunctions, this)
        ),
        MenuItem(
            tr("Test random number functions (and associated floating point "
               "assistance functions)"),
            std::bind(&TestMenu::testRandom, this)
        ),
        MenuItem(
            tr("Test logistic regression, and the underlying generalized linear model (GLM)"),
            std::bind(&TestMenu::testLogisticRegression, this)
        ),
        MAKE_MENU_MENU_ITEM(WidgetTestMenu, app),
        // MenuItem(
        //     tr("(†) Run software unit tests (reporting to debugging stream)")
        // ).setNeedsPrivilege(),
    };
}


TestMenu::~TestMenu()
{
    // Unsure if necessary - but similar code in QuAudioPlayer was crashing.
    soundfunc::finishMediaPlayer(m_player);
}


void TestMenu::testDebugConsole()
{
    qInfo() << "Testing debug console. This is the entire test. Success.";
    uifunc::alert("Success! See the debug console for output.");
}


void TestMenu::testSound()
{
    soundfunc::makeMediaPlayer(m_player);
    QUrl url(uiconst::DEMO_SOUND_URL_1);
    qDebug() << "Trying to play:" << url;
    m_player->setMedia(url);
    m_player->setVolume(50);
    m_player->play();
}


void TestMenu::testHttps()
{
    // To find bad certificates, see
    // https://www.ssllabs.com/ssltest/analyze.html
    QString url = "https://egret.psychol.cam.ac.uk/index.html";  // good cert
    // QString url = "https://www.veltigroup.com/";  // bad cert (then Forbidden)

    NetworkManager* netmgr = m_app.networkManager();
    netmgr->setTitle("Test HTTPS");
    netmgr->testHttpsGet(url);
}


void TestMenu::testHttp()
{
    QString url = "http://egret.psychol.cam.ac.uk/index.html";
    NetworkManager* netmgr = m_app.networkManager();
    netmgr->setTitle("Test HTTP");
    netmgr->testHttpGet(url);
}


void TestMenu::testPhq9Creation()
{
#ifdef DEBUG_OPTIONS
    QString tablename = Phq9::PHQ9_TABLENAME;
    TaskPtr p_task = m_app.taskFactory()->create(tablename);
    if (!p_task) {
        qCritical() << Q_FUNC_INFO << "Failed to create task: "
                    << qUtf8Printable(tablename);
        return;
    }
    qDebug() << *p_task;
    doneSeeConsole();
#endif
}


void TestMenu::testIcd10CodeSetCreation()
{
#ifdef DEBUG_OPTIONS
    Icd10 icd(m_app);
    qDebug() << icd;
    doneSeeConsole();
#endif
}


void TestMenu::testIcd9cmCodeSetCreation()
{
#ifdef DEBUG_OPTIONS
    Icd9cm icd(m_app);
    qDebug() << icd;
    doneSeeConsole();
#endif
}


void TestMenu::doneSeeConsole()
{
    if (platform::PLATFORM_TABLET) {
        uifunc::alert("Done; see USB debugging output");
    } else {
        uifunc::alert("Done; see console");
    }
}


void TestMenu::testProgress()
{
    qDebug() << Q_FUNC_INFO << "start";
    // http://doc.qt.io/qt-4.8/qprogressdialog.html#details
    // http://stackoverflow.com/questions/3752742/how-do-i-create-a-pause-wait-function-using-qt
    int num_things = 100;
    QProgressDialog progress(
        "Testing progress (but not doing anything; safe to abort)...",
        "Abort test", 0, num_things, this);
    progress.setWindowTitle("Progress dialog");
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    for (int i = 0; i < num_things; i++) {
        progress.setValue(i);
        if (progress.wasCanceled()) {
            break;
        }
        // Do a small thing:
        QThread::msleep(50);
        // Prevent other things (like audio) from freezing:
        QCoreApplication::processEvents();
    }
    progress.setValue(num_things);
    qDebug() << Q_FUNC_INFO << "finish";
}


void TestMenu::testWait()
{
    SlowNonGuiFunctionCaller(
        std::bind(&TestMenu::expensiveFunction, this),
        this,
        QString("Running expensive function in worker thread (for %1 "
                "ms)").arg(EXPENSIVE_FUNCTION_DURATION_MS),
        "Please wait");
}


void TestMenu::expensiveFunction()
{
    qDebug() << Q_FUNC_INFO << "start: sleep time (ms)" << EXPENSIVE_FUNCTION_DURATION_MS;
    QThread::msleep(EXPENSIVE_FUNCTION_DURATION_MS);
    qDebug() << Q_FUNC_INFO << "finish";
}


void TestMenu::testSizeFormatter()
{
    bool space = true;
    bool longform = false;
    QString suffix = longform ? "bytes" : "B";
    QVector<double> nums{
        3e0, 3e1, 3e2, 3e3, 3e4, 3e5, 3e6, 3e7, 3e8, 3e9,
        3e10, 3e11, 3e12, 3e13, 3e14, 3e15, 3e16, 3e17, 3e18, 3e19,
        3e20, 3e21, 3e22, 3e23, 3e24, 3e25, 3e26, 3e27, 3e28, 3e29,
        0, 27, 999, 1000, 1023, 1024, 1728, 110592, 7077888, 452984832,
        28991029248, 1855425871872, 9223372036854775807.0};
    QString text;
    for (bool binary : {false, true}) {
        for (auto num : nums) {
            text += QString("%1 → %2\n")
              .arg(num)
              .arg(convert::prettySize(num, space, binary, longform, suffix));
        }
    }
    uifunc::alertLogMessageBox(text, "Size formatting", false);
}


void TestMenu::testConversions()
{
    convert::testConversions();
    uifunc::alert("Conversion test: OK");
}


void TestMenu::testEigenFunctions()
{
    QString text = eigenfunc::testEigenFunctions().join("\n");

    uifunc::alertLogMessageBox(text, "Eigen functions successfully tested",
                               false);
}


void TestMenu::testRandom()
{
    QString text = ccrandom::testRandom().join("\n");
    uifunc::alertLogMessageBox(text,
                               "Random-number functions (and supporting "
                               "floating-point-delta functions): OK",
                               false);
}


void TestMenu::testLogisticRegression()
{
    using namespace eigenfunc;
    using namespace Eigen;
    QStringList results;

    // ========================================================================
    // Data set 1
    // ========================================================================

    qInfo() << Q_FUNC_INFO
            << "1a. Our 'plain' method: LogisticDescriptives(x, y)";
    QVector<double> x_q{0.50, 0.75, 1.00, 1.25, 1.50, 1.75, 1.75, 2.00, 2.25, 2.50, 2.75, 3.00, 3.25, 3.50, 4.00, 4.25, 4.50, 4.75, 5.00, 5.50};
    QVector<int> y_q{0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1};
    LogisticDescriptives ld1(x_q, y_q, true);
    QString result1 = QString(R"(
# Example from: https://en.wikipedia.org/wiki/Logistic_regression
# R code:

d <- data.frame(
    x = c(0.50, 0.75, 1.00, 1.25, 1.50, 1.75, 1.75, 2.00, 2.25, 2.50, 2.75, 3.00, 3.25, 3.50, 4.00, 4.25, 4.50, 4.75, 5.00, 5.50),
    y = c(0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1)
)
dm <- matrix(c(rep(1, length(d$x)), d$x), ncol=2)
# irls_svdnewton(dm, d$y)
model <- glm(y ~ x, family=binomial(link='logit'), data=d)
summary(model)

# R gives coefficients: intercept = -4.0777, x = 1.5046
# (as per Wikipedia also)

Our results: intercept = %1, slope = %2)
        )").arg(ld1.intercept()).arg(ld1.slope());

    qInfo() << Q_FUNC_INFO
            << "1b. A more detailed look: LogisticRegression(), IRLS";
    VectorXd x_e = eigenColumnVectorFromQVector<double>(x_q);
    VectorXi y_e = eigenColumnVectorFromQVector<int>(y_q);
    LogisticRegression lr1a(Glm::SolveMethod::IRLS_KaneLewis);
    lr1a.setVerbose(true);
    lr1a.fit(x_e, y_e);
    VectorXd coeffs1a = lr1a.coefficients();
    VectorXd p = lr1a.predictProb();
    VectorXi cat = lr1a.predictBinary();

    qInfo() << Q_FUNC_INFO
            << "1c. A more detailed look: LogisticRegression(), IRLS SVD Newton";
    LogisticRegression lr1c(Glm::SolveMethod::IRLS_SVDNewton_KaneLewis);
    lr1c.setVerbose(true);
    lr1c.fit(x_e, y_e);
    VectorXd coeffs1c = lr1c.coefficients();

    results.append(QString(R"(
With the same data:

IN x: %1
IN y: %2

IRLS method:
OUT coefficients: %3
OUT predicted p: %4
OUT predicted categories: %5
OUT n_iterations: %6
OUT time to fit (ms): %7

IRLS-SVD-Newton method:
OUT coefficients: %8
OUT n_iterations: %9
OUT time to fit (ms): %10
        )")
            .arg(qStringFromEigenMatrixOrArray(x_e))
            .arg(qStringFromEigenMatrixOrArray(y_e))
            .arg(qStringFromEigenMatrixOrArray(coeffs1a))
            .arg(qStringFromEigenMatrixOrArray(p))
            .arg(qStringFromEigenMatrixOrArray(cat))
            .arg(QString::number(lr1a.nIterations()))
            .arg(QString::number(lr1a.timeToFitMs()))
            .arg(qStringFromEigenMatrixOrArray(coeffs1c))
            .arg(QString::number(lr1c.nIterations()))
            .arg(QString::number(lr1c.timeToFitMs())));

    VectorXd test_x = eigenColumnVectorFromInitList<double>({0.8, 1.6, 2.4, 3.2});
    VectorXd predicted_p = lr1a.predictProb(test_x);
    VectorXd retrieved_x = lr1a.retrodictUnivariatePredictor(predicted_p);
    LogisticDescriptives ld2(coeffs1a);
    VectorXd crosscheck_x(retrieved_x.size());
    for (int i = 0; i < crosscheck_x.size(); ++i) {
        double p = predicted_p(i);
        crosscheck_x(i) = ld2.x(p);
    }

#ifdef GLM_OFFER_R_GLM_FIT
    qInfo() << Q_FUNC_INFO
            << "1d. LogisticRegression(), IRLS implemented as per R glm.fit";
    LogisticRegression lr1d(Glm::SolveMethod::IRLS_R_glmfit);
    lr1d.setVerbose(true);
    lr1d.fit(x_e, y_e);
    VectorXd coeffs1d = lr1d.coefficients();
    results.append(QString("With our implementation of R's glm.fit IRLS: "
                           "%1").arg(qStringFromEigenMatrixOrArray(coeffs1d)));
#endif

    results.append(QString(R"(
Now some silly things:

test_x: %1
predicted_p: %2
retrieved_x [SHOULD MATCH test_x]: %3
crosscheck_x (via LogisticDescriptives()) [SHOULD MATCH test_x]: %4
        )")
            .arg(qStringFromEigenMatrixOrArray(test_x))
            .arg(qStringFromEigenMatrixOrArray(predicted_p))
            .arg(qStringFromEigenMatrixOrArray(retrieved_x))
            .arg(qStringFromEigenMatrixOrArray(crosscheck_x)));

    // ========================================================================
    // Data set 2
    // ========================================================================

    qInfo() << Q_FUNC_INFO
            << "2a. A more numerically complex example, via IRLS.";
    VectorXd x2 = eigenColumnVectorFromInitList<double>({
        0.09969334049243989, 0, 0.04984667024621994,
        0.059846670246219945, 0.04984667024621994, 0.059846670246219945,
        0.04984667024621994, 0.059846670246219945, 0.04984667024621994,
        0.059846670246219945, 0.06984667024621995, 0.059846670246219945,
        0.06984667024621995, 0.059846670246219945});
    VectorXi y2 = eigenColumnVectorFromInitList<int>({
        1, 0, 0,
        1, 0, 1,
        0, 1, 0,
        0, 1, 0,
        1, 1});
    LogisticRegression lr2a(Glm::SolveMethod::IRLS_KaneLewis);
    lr2a.setVerbose(true);
    lr2a.fit(x2, y2);
    VectorXd coeffs2a = lr2a.coefficients();

    qInfo() << Q_FUNC_INFO
            << "2b. A more numerically complex example, via IRLS-SVD-Newton.";
    LogisticRegression lr2b(Glm::SolveMethod::IRLS_SVDNewton_KaneLewis);
    lr2b.setVerbose(true);
    lr2b.fit(x2, y2);
    VectorXd coeffs2b = lr2b.coefficients();

    results.append(QString(R"(
Another example, giving a warning in R:

d2 <- data.frame(
    intensity = c(
        0.09969334049243989, 0, 0.04984667024621994,
        0.059846670246219945, 0.04984667024621994, 0.059846670246219945,
        0.04984667024621994, 0.059846670246219945, 0.04984667024621994,
        0.059846670246219945, 0.06984667024621995, 0.059846670246219945,
        0.06984667024621995, 0.059846670246219945
    ),
    yes = c(
        1, 0, 0,
        1, 0, 1,
        0, 1, 0,
        0, 1, 0,
        1, 1)
)
m2 <- glm(yes ~ intensity, family=binomial(link='logit'), data=d2)
# R coefficients: intercept -119.8, slope 2014.1

CamCOPS: coefficients: IRLS: %1
CamCOPS: coefficients: IRLS-SVD-Newton: %2
)")
            .arg(qStringFromEigenMatrixOrArray(coeffs2a))
            .arg(qStringFromEigenMatrixOrArray(coeffs2b)));

#ifdef GLM_OFFER_R_GLM_FIT
    qInfo() << Q_FUNC_INFO << "2c. And again with the R glm.fit method.";
    LogisticRegression lr2c(Glm::SolveMethod::IRLS_R_glmfit);
    lr2c.setVerbose(true);
    lr2c.fit(x2, y2);
    VectorXd coeffs2c = lr2c.coefficients();
    results.append(QString(
           "CamCOPS: coefficients: RNC implementation of R's "
           "glm.fit IRLS: %1").arg(qStringFromEigenMatrixOrArray(coeffs2c)));
#endif

    /*

plot(d2$intensity, d2$yes)
m2 <- glm(yes ~ intensity, family=binomial(link='logit'), data=d2)
# Warning message: glm.fit: fitted probabilities numerically 0 or 1 occurred
# R coefficients: intercept -119.8, slope 2014.1
predict(m2, type='response')

# Then with IRLS-SVD-Newton method from https://bwlewis.github.io/GLM/ :
design2 = matrix(c(rep(1, length(d2$intensity)), d2$intensity), ncol=2)
m3 <- irls_svdnewton(design2, d2$yes)  # maxit=25, tol=1e-08
# ... coefficients -16.69754, 240.95452; iterations 25 (i.e. non-convergence)

We get non-convergence with IRLS, but with the same max_iterations and
tolerance, we get the same results from IRLS-SVD-Newton:

CamCOPS: coefficients: IRLS: %1
CamCOPS: coefficients: IRLS-SVD-Newton: %2

... but note that max_iterations is important during non-convergence; e.g. with
500 instead, we get 0.177316312229748, 2.10120649598573. Stick with 25!

To visualize:

logistic <- function(x, intercept=0, slope=1) {
  t <- intercept + slope * x
  1 / (1 + exp(-t))
}
# curve(logistic(x), -6, 6)  # as per Wikipedia!
r_fn <- function(x) logistic(x, -119.8, 2014.1)
camcops_25_fn <- function(x) logistic(x, -16.6975412143982, 240.954480219989)
camcops_500_fn <- function(x) logistic(x, 0.177316312229748, 2.10120649598573)
rcppnumerical_fastlr_fn <- function(x) logistic(x, -49.08323, 831.76727)
plot(r_fn, 0, max(d2$intensity), col='blue')
plot(camcops_25_fn, col='red', add=TRUE)
plot(camcops_500_fn, col='green', add=TRUE)
plot(rcppnumerical_fastlr_fn, col='orange', add=TRUE)
points(x=d2$intensity, y=d2$yes)

The R function is doing it better, although the IRLS-SVD-Newton one isn't dreadful.
To see R's actual GLM method, type "glm.fit".
See also https://www.r-bloggers.com/even-faster-linear-model-fits-with-r-using-rcppeigen/
// *** Implement the full method used by R, or RcppEigen

https://github.com/RcppCore/RcppEigen/blob/master/src/fastLm.cpp
https://github.com/yixuan/RcppNumerical/blob/master/src/fastLR.cpp
https://github.com/lme4/lme4/tree/master/src

Rcpp

    */

    uifunc::alertLogMessageBox(results.join("\n"),
                               tr("Test logistic regression"), false);
}
