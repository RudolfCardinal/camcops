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

#pragma once
#include <QSharedPointer>
#include "menulib/menuwindow.h"
#include "questionnairelib/namevalueoptions.h"
#include "widgets/booleanwidget.h"

class Blob;
class QCustomPlot;
class QSizePolicy;
class QuElement;


class WidgetTestMenu : public MenuWindow
{
    Q_OBJECT
public:
    WidgetTestMenu(CamcopsApp& app);
    virtual QString title() const override;
protected:
    virtual void makeItems() override;
    QVariant dummyGetter1() const;
    bool dummySetter1(const QVariant& value);
    QVariant dummyGetter2() const;
    bool dummySetter2(const QVariant& value);
    void dummyAction();
    void testQuestionnaireElement(QuElement* element);

    // ========================================================================
    // Endogenous Qt widgets
    // ========================================================================
    void testQLabel(const QSizePolicy& policy,
                    bool long_text, bool word_wrap);
    void testQPushButton(const QSizePolicy& policy);

    // ========================================================================
    // Low-level widgets
    // ========================================================================
    void testAdjustablePie(int n, bool rotate_labels);
    void testAspectRatioPixmap();
    void testBooleanWidget(BooleanWidget::Appearance appearance,
                           bool long_text);
    // Camera: use QuPhoto instead
    void testCanvasWidget(bool allow_shrink);
    void testClickableLabelNoWrap(bool long_text);
    void testClickableLabelWordWrapWide(bool long_text);
    // DiagnosticCodeSelector: use QuDiagnosticCode instead
    void testFixedAreaHfwTestWidget();
    void testFixedAspectRatioHfwTestWidget();
    void testFixedNumBlocksHfwTestWidget();
    // GrowingTextEdit: see QuTextEdit
    void testHorizontalLine();
    void testImageButton();
    void testLabelWordWrapWide(bool long_text, bool use_hfw_layout,
                               bool with_icons = false);
    // OpenableWidget: part of main app framework instead
    // Spacer: see QuSpacer instead
    void testSvgWidgetClickable();
    void testThermometer();
    // TickSlider: see QuSlider instead
    void testVerticalLine();

    // ========================================================================
    // Layouts and the like
    // ========================================================================
    void testFlowLayout(int n, bool text, Qt::Alignment halign);
    void testFlowLayoutFixedNumBlocksHfwTestWidget(int n);
    void testFlowLayoutMixture();
    void testBaseWidget(bool long_text);
    void testVBoxLayout(bool long_text);
    void testHBoxLayoutHfwStretch();
    void testGridLayoutHfw(int example);
    void testVerticalScrollAreaSimple();
    void testVerticalScrollAreaComplex(bool long_text);
    void testVerticalScrollAreaFixedAreaHfwWidget();
    void testVerticalScrollAreaAspectRatioPixmap();
    void testVerticalScrollGridLayout();

    // ========================================================================
    // Large-scale widgets
    // ========================================================================
    void testMenuItem();
    void testQuestionnaireHeader();
    void testQuestionnaire(bool long_title, bool as_openable_widget);
    // void testAce3();

    // ========================================================================
    // Questionnaire element widgets
    // ========================================================================
    void testQuAudioPlayer();
    void testQuBoolean(bool as_text_button, bool long_text,
                       bool false_appears_blank);
    void testQuButton();
    void testQuCanvas();
    void testQuCountdown();
    void testQuDateTime();
    void testQuDateTimeLimited();
    void testQuDiagnosticCode();
    void testQuHeading(bool long_text);
    void testQuHorizontalLine();
    void testQuImage();
    void testQuLineEdit();
    void testQuLineEditDouble();
    void testQuLineEditInteger();
    void testQuLineEditLongLong();
    void testQuLineEditNHSNumber();
    void testQuLineEditULongLong();  // deprecated as SQLite3 can't cope
    void testQuMCQ(bool horizontal, bool long_text, bool as_text_button);
    void testQuMCQGrid(bool expand, int example);
    void testQuMCQGridDouble(bool expand);
    void testQuMCQGridSingleBoolean(bool expand);
    void testQuMultipleResponse(bool horizontal, bool long_text);
    void testQuPhoto();
    void testQuPickerInline();
    void testQuPickerPopup();
    void testQuSlider(bool horizontal);
    void testQuSpacer();
    void testQuSpinBoxDouble();
    void testQuSpinBoxInteger();
    void testQuText(bool long_text, bool bold);
    void testQuTextEdit();
    void testQuThermometer();

    // ========================================================================
    // Graphs
    // ========================================================================
    // Make a QCustomPlot and return it; warn if creation failed.
    QCustomPlot* makeQCustomPlotOrWarn();

    // Take ownership of the plot pointer; show a dialogue with the plot in.
    void showPlot(QCustomPlot* p, const QSize& minsize = QSize(300, 300));

    // Plot testing
    void testQCustomPlot1();
    void testQCustomPlot2();

protected:
    FieldRefPtr m_fieldref_1;
    FieldRefPtr m_fieldref_2;
    BlobFieldRefPtr m_fieldref_blob;
    QVariant m_dummy_value_1;
    QVariant m_dummy_value_2;
    NameValueOptions m_options_1;
    NameValueOptions m_options_2;
    NameValueOptions m_options_3;
    QSharedPointer<Blob> m_blob;
};
