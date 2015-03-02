#include "widget/woverviewrgb.h"

#include <QPainter>

#include "util/timer.h"
#include "util/math.h"
#include "waveform/waveform.h"

WOverviewRGB::WOverviewRGB(const char* pGroup,
                           ConfigObject<ConfigValue>* pConfig, QWidget* parent)
        : WOverview(pGroup, pConfig, parent)  {
}

bool WOverviewRGB::drawNextPixmapPart() {
    ScopedTimer t("WOverviewRGB::drawNextPixmapPart");

    //qDebug() << "WOverview::drawNextPixmapPart() - m_waveform" << m_waveform;

    int currentCompletion;

    ConstWaveformPointer pWaveform = getWaveform();
    if (!pWaveform) {
        return false;
    }

    const int dataSize = pWaveform->getDataSize();
    if (dataSize == 0) {
        return false;
    }

    if (!m_pWaveformSourceImage) {
        // Waveform pixmap twice the height of the viewport to be scalable
        // by total_gain
        // We keep full range waveform data to scale it on paint
        m_pWaveformSourceImage = new QImage(dataSize / 2, 2 * 255,
                QImage::Format_ARGB32_Premultiplied);
        m_pWaveformSourceImage->fill(QColor(0,0,0,0).value());
    }

    // Always multiple of 2
    const int waveformCompletion = pWaveform->getCompletion();
    // Test if there is some new to draw (at least of pixel width)
    const int completionIncrement = waveformCompletion - m_actualCompletion;

    int visiblePixelIncrement = completionIncrement * width() / dataSize;
    if (completionIncrement < 2 || visiblePixelIncrement == 0) {
        return false;
    }

    const int nextCompletion = m_actualCompletion + completionIncrement;

    //qDebug() << "WOverview::drawNextPixmapPart() - nextCompletion:"
    //         << nextCompletion
    //         << "m_actualCompletion:" << m_actualCompletion
    //         << "waveformCompletion:" << waveformCompletion
    //         << "completionIncrement:" << completionIncrement;

    QPainter painter(m_pWaveformSourceImage);
    painter.translate(0.0,(double)m_pWaveformSourceImage->height()/2.0);

    QColor color;

    qreal lowColor_r, lowColor_g, lowColor_b;
    m_signalColors.getRgbLowColor().getRgbF(&lowColor_r, &lowColor_g, &lowColor_b);

    qreal midColor_r, midColor_g, midColor_b;
    m_signalColors.getRgbMidColor().getRgbF(&midColor_r, &midColor_g, &midColor_b);

    qreal highColor_r, highColor_g, highColor_b;
    m_signalColors.getRgbHighColor().getRgbF(&highColor_r, &highColor_g, &highColor_b);

    // "Raw" LMH values
    qreal low, mid, high;

    // Non-normalized RGB values
    qreal red, green, blue;

    // Maximum is needed for normalization
    qreal max;

    for (currentCompletion = m_actualCompletion;
            currentCompletion < nextCompletion; currentCompletion += 2) {

        unsigned char left = pWaveform->getAll(currentCompletion);
        unsigned char right = pWaveform->getAll(currentCompletion + 1);

        low  = (qreal) pWaveform->getLow(currentCompletion);
        mid  = (qreal) pWaveform->getMid(currentCompletion);
        high = (qreal) pWaveform->getHigh(currentCompletion);

        // Do matrix multiplication
        red   = low * lowColor_r + mid * midColor_r + high * highColor_r;
        green = low * lowColor_g + mid * midColor_g + high * highColor_g;
        blue  = low * lowColor_b + mid * midColor_b + high * highColor_b;

        // Normalize and draw
        max = math_max3(red, green, blue);
        if (max > 0.0f) {
            color.setRgbF(red / max, green / max, blue / max);
            painter.setPen(color);
            painter.drawLine(currentCompletion / 2, -left, currentCompletion / 2, 0);
        }

        low  = (qreal) pWaveform->getLow(currentCompletion + 1);
        mid  = (qreal) pWaveform->getMid(currentCompletion + 1);
        high = (qreal) pWaveform->getHigh(currentCompletion + 1);

        // Do matrix multiplication
        red   = low * lowColor_r + mid * midColor_r + high * highColor_r;
        green = low * lowColor_g + mid * midColor_g + high * highColor_g;
        blue  = low * lowColor_b + mid * midColor_b + high * highColor_b;

        // Normalize and draw
        max = math_max3(red, green, blue);
        if (max > 0.0f) {
            color.setRgbF(red / max, green / max, blue / max);
            painter.setPen(color);
            painter.drawLine(currentCompletion / 2, 0, currentCompletion / 2, right);
        }
    }

    // Evaluate waveform ratio peak
    for (currentCompletion = m_actualCompletion;
            currentCompletion < nextCompletion; currentCompletion += 2) {
        m_waveformPeak = math_max3(
                m_waveformPeak,
                static_cast<float>(pWaveform->getAll(currentCompletion)),
                static_cast<float>(pWaveform->getAll(currentCompletion + 1)));
    }

    m_actualCompletion = nextCompletion;
    m_waveformImageScaled = QImage();
    m_diffGain = 0;

    // Test if the complete waveform is done
    if (m_actualCompletion >= dataSize - 2) {
        m_pixmapDone = true;
        //qDebug() << "m_waveformPeakRatio" << m_waveformPeak;
    }

    return true;
}
