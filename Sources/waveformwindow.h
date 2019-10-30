/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/
#ifndef WAVEFORMWINDOW_H
#define WAVEFORMWINDOW_H

#include <QWidget>
#include "patternelement.h"
#include "PtnImage.h"

/**
 * Class for displaying the pattern preview window
 * This provides interface for displaying and seleting pattern images and
 * trigger waveforms.
 */
class WaveFormWindow : public QWidget
{
    Q_OBJECT
public:

    /**
     * Enumartion for seleting the pattern
     */
    enum SelectType
    {
        SELECT_NONE,
        SELECT_SINGLE,
        SELECT_CONTINUE,
        SELECT_MULTIPLE,
        SELECT_ALL
    };

    /**
     * Enumartion for the trigger types
     */
    enum InTriggerType
    {
        TRIG_IN_EXT_POSITIVE,
        TRIG_IN_EXT_NEGATIVE
    };

    explicit WaveFormWindow(QWidget *parent = 0);

    void setZoom(int zoom);
    void select(SelectType type, int index);
    void draw(void);
    void updateInvertTrigOut1(bool value)
    {
        m_invertTrigOut1 = value;
    }
    void updateInvertTrigOut2(bool value)
    {
        m_invertTrigOut2 = value;
    }
    void updatePatternList(QList<PatternElement> patElem)
    {
        m_elements = patElem;
    }
    void updateTriggerInType(int value)
    {
        m_triggerInType = InTriggerType(value);
    }
    void updateVideoPatternMode(bool value)
    {
        m_videoPatternMode = value;
    }

    void setPatternSize(int width, int height);

signals:
    void selectionChange(int index, QList<PatternElement> patElem);

private:
    enum
    {
        MAX_PTN_CACHE = 16
    };
    class PtnImageCache
    {
    public:
        QImage *img;
        QString fileName;
        int bitDepth;
        int access;
        PtnImageCache()
        {
            bitDepth = 0;
            access = 0;
            img = NULL;
            fileName = "";
        }
    };

    void setSelection(int index);
    int positionToPattern(int x, int y);
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *event);
    int findNextSelection(void);
    QImage *loadImage(QString name, int bitDepth);
    int quantize(QImage const &src, int bitDepth, QImage *dst);
    double m_scale;
    int m_selectedPattern;
    bool m_invertTrigOut1;
    bool m_invertTrigOut2;
    bool m_videoPatternMode;
    InTriggerType m_triggerInType;
    QList<PatternElement> m_elements;
    int m_accessCounter;
    int m_ptnWidth;
    int m_ptnHeight;
    PtnImageCache m_ptnImageCache[MAX_PTN_CACHE];
};
#endif // WAVEFORMWINDOW_H
