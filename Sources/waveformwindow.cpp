/*
 * This file lists the WaveDrawer and WaveFormWindow classes
 * These classes are for painting the Pattern Images on the Patternd design page
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QWidget>
#include <QTextStream>
#include "waveformwindow.h"

static const int MIN_WAVEFORMWINDOW_SIZE=431;
static const int PATTERN_GAP = 16;
static const int WAVE_HEIGHT = 30;
static const int PTN_IMG_HEIGHT = 150;
static const int PTN_IMG_WIDTH = 1500;
static const int ELEMENT_GAP = 20;
static const int TRIGGER_WIDTH = 20;
static const int PTN_NUM_WIN_HEIGHT = 20;
static const int COLOR_WIN_HEIGHT = 30;
static const int TOTAL_HEIGHT = WAVE_HEIGHT * 3 + ELEMENT_GAP * 6 +
        PTN_NUM_WIN_HEIGHT + PTN_IMG_HEIGHT + COLOR_WIN_HEIGHT;

/**
 * Class for drawing tirgger waveforms. This can be used inside the paint
 * even to draw the wave form on a canvas
 *
 */
class WaveDrawer
{
public:
    WaveDrawer(int x0_, int y0_, int width_, int height_)
    {
        x0 = x0_;
        y0 = y0_;
        width = width_;
        height = height_;
    }

    int getPos() const
    {
        return x;
    }

    void start(QPainter *painter_, bool value)
    {
        x = x0;
        y = value ? y0 : height + y0;
        first = true;

        painter = painter_;
        painter->setPen(QColor(0,0,0));
        painter->setBrush(Qt::NoBrush);
    }

    void skipTo(int newX)
    {
        x = x0 + newX;
    }

    int draw(bool value, int size, QColor color)
    {
        if(size <= 0)
            return 0;

        if(x >= x0 + width)
            return -1;

        if(x + size >= x0 + width)
            size = x0 + width - x;

        int y1 = y;
        y = (value) ? y0 : y0 + height;

        painter->setPen(color);

        if(!first && y != y1)
        {
            painter->drawLine(x - 1, y1, x + 1, y);
            x += 1;
            size -= 1;
        }

        painter->drawLine(x, y, x + size - 1, y);
        x = x + size;
        first = false;
        return 0;
    }

private:
    int x0;
    int y0;
    int width;
    int height;
    int x;
    int y;
    bool first;
    QPainter *painter;
};

/**
 * Constructure for the WaveFormWindow
 * @param parent Parent widget
 */
WaveFormWindow::WaveFormWindow(QWidget *parent) :
    QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_scale = 0.1;
    m_selectedPattern = -1;
    m_triggerInType = TRIG_IN_EXT_POSITIVE;
    m_invertTrigOut1 = m_invertTrigOut2 = false;
    m_videoPatternMode = false;
    m_accessCounter = 0;
    m_ptnWidth = 0;
    m_ptnHeight = 0;
    setAttribute(Qt::WA_OpaquePaintEvent);
}

/**
 * This should be called whenever the pattern window need to be updated
 */
void WaveFormWindow::draw(void)
{
    int totalWidth;
    totalWidth = PTN_IMG_WIDTH * m_scale * m_elements.size() + PATTERN_GAP * 2* m_elements.size();
    setMinimumSize(totalWidth, TOTAL_HEIGHT);
    update();
}

/**
 * Paint event for the pattern preview widget. This will draw all the content
 * of the pattern preview from the m_elements array
 */
void WaveFormWindow::paintEvent(QPaintEvent *)
{
    QPainter painter;
    int x = 0;
    QColor selectColor1 = QColor(0x80, 0x80, 0x80);
    QColor selectColor2 = QColor(0x40, 0x40, 0x40);

    int gap = 25 + (this->height() - MIN_WAVEFORMWINDOW_SIZE)/6;

    QRect visibleRect = visibleRegion().boundingRect();
    
    WaveDrawer waveTrigIn(0, PTN_NUM_WIN_HEIGHT + PTN_IMG_HEIGHT + COLOR_WIN_HEIGHT + gap * 3, width(), WAVE_HEIGHT);
    WaveDrawer waveTrigOut1(0, PTN_NUM_WIN_HEIGHT + PTN_IMG_HEIGHT + COLOR_WIN_HEIGHT + gap * 4 + WAVE_HEIGHT, width(), WAVE_HEIGHT);
    WaveDrawer waveTrigOut2(0, PTN_NUM_WIN_HEIGHT + PTN_IMG_HEIGHT + COLOR_WIN_HEIGHT + gap * 5 + WAVE_HEIGHT * 2, width(), WAVE_HEIGHT);
    
    painter.begin(this);
    painter.fillRect(0, 0, width(), height(), QColor(0,0,0));


    if(m_elements.size() > 0)
    {
        waveTrigIn.start(&painter, 0);
        waveTrigOut1.start(&painter, 0);
        waveTrigOut2.start(&painter, 0);

        for(int i = 0; i < m_elements.size(); i++)
        {
            PatternElement *e = &m_elements[i];
            bool insertDark = false;
            //if (i)
            //{
                //PatternElement *ePrev = &m_elements[i - 1];
                //if (ePrev->darkPeriod)
                    //insertDark = true;
            //}
            // test
            if (e->darkPeriod)
                insertDark = true;
            //

            int imgWin_x, imgWin_width;
            int width = PTN_IMG_WIDTH * m_scale + PATTERN_GAP + PATTERN_GAP;
            QRect rect(x, 0, width, height());

            if(!visibleRect.intersects(rect))
            {
                x += width;
                continue;
            }

            if(e->selected)
            {
                painter.fillRect(rect.adjusted(2,2,-2,-2), selectColor2);
            }

            if ((e->trigIn) && (insertDark))
            {
                imgWin_x = rect.x() + PATTERN_GAP;
                imgWin_width = rect.width() - PATTERN_GAP - PATTERN_GAP;
            }
            else if (e->trigIn)
            {
                imgWin_x = rect.x() + PATTERN_GAP;
                imgWin_width = rect.width() - PATTERN_GAP;
            }
            else if (insertDark)
            {
                imgWin_x = rect.x();
                imgWin_width = rect.width() - PATTERN_GAP;
            }
            else
            {
                imgWin_x = rect.x();
                imgWin_width = rect.width();
            }
            QRect textRect(imgWin_x, rect.y(), imgWin_width, PTN_NUM_WIN_HEIGHT);
            QRect imgRect(imgWin_x, rect.y() + PTN_NUM_WIN_HEIGHT, imgWin_width, PTN_IMG_HEIGHT);
            QRect colorRect(imgWin_x, rect.y() + PTN_NUM_WIN_HEIGHT + PTN_IMG_HEIGHT + gap * 2, imgWin_width, COLOR_WIN_HEIGHT);

            painter.setPen(selectColor1);
            painter.drawRect(textRect);
            painter.drawText(textRect, Qt::AlignCenter, QString::number(i));

            painter.drawRect(imgRect);

            if (m_videoPatternMode)
            {
                double bitPosBlock_width = (imgRect.width() - 4) / 8.0;
                double bitPosBlock_height = (imgRect.height() - 4) / 3.0;
				char ColorName[] = "GRB";
				QColor RGBColor[3];
                QColor RGBColorDark[3];
				RGBColor[0] = QColor(  0, 255,   0);
				RGBColor[1] = QColor(255,   0,   0);
				RGBColor[2] = QColor(  0,   0, 255);
                RGBColorDark[0] = QColor(  0, 100,   0);
                RGBColorDark[1] = QColor(100,   0,   0);
                RGBColorDark[2] = QColor(  0,   0, 100);

                painter.setPen(QColor(0,0,0));
                for (int i = 0; i < 24; i++)
                {
                    double x = imgRect.x() + 2 + bitPosBlock_width*(i%8);
                    double y = imgRect.y() + 2 + bitPosBlock_height*(i/8);
                    int bitPosBlock_x = x + 0.5;
                    int bitPosBlock_y = y + 0.5;
                    int width = x + bitPosBlock_width - bitPosBlock_x + 0.5;

                    QRect bitPosRect(bitPosBlock_x, bitPosBlock_y, width, bitPosBlock_height);
                    char dispStr[10];

                    painter.drawRect(bitPosRect);
                    if (e->splashImageBitPos <= i && i < e->splashImageBitPos + e->bits)
                        painter.fillRect(bitPosRect.adjusted(1,1,-1,-1), RGBColor[i/8]);
                    else
                        painter.fillRect(bitPosRect.adjusted(1,1,-1,-1), RGBColorDark[i/8]);

                    sprintf(dispStr, "%c\n%d", ColorName[i/8], i);
                    painter.drawText(bitPosRect, Qt::AlignCenter,tr(dispStr));
                    bitPosBlock_x += bitPosBlock_width;
                }
            }
            else
            {
                QImage *scaledImage = loadImage(m_elements[i].name, m_elements[i].bits);
                if(scaledImage)
                {
                    if (imgRect.width() > scaledImage->width())
                    {
                        int scaledImage_width = scaledImage->width();
                        int scaledImage_x = ((2 * rect.x() + rect.width()) / 2) - scaledImage_width / 2;
                        QRect scaledImage_rect(scaledImage_x, rect.y() + PTN_NUM_WIN_HEIGHT, scaledImage_width, PTN_IMG_HEIGHT);
                        painter.setPen(QColor(0,0,0));
                        painter.drawRect(scaledImage_rect.adjusted(0,2,0,-2));
                        painter.drawImage(scaledImage_rect.adjusted(0,2,0,-2), *scaledImage);
                        painter.setPen(selectColor1);
                    }
                    else
                    {
                        painter.drawImage(imgRect.adjusted(2,2,-2,-2), *scaledImage);
                    }
                }
            }

            painter.fillRect(colorRect, e->getColor());

            waveTrigIn.skipTo(x);
            waveTrigOut1.skipTo(x);
            waveTrigOut2.skipTo(x);

            if (e->trigIn)
            {
                switch(m_triggerInType)
                {
                case TRIG_IN_EXT_POSITIVE:
                    waveTrigIn.draw(0, PATTERN_GAP/2, Qt::white);
                    waveTrigIn.draw(1, PATTERN_GAP, Qt::white);
                    waveTrigIn.draw(0, width - (PATTERN_GAP + PATTERN_GAP/2), Qt::white);
                    break;
                case TRIG_IN_EXT_NEGATIVE:
                    waveTrigIn.draw(1, PATTERN_GAP, Qt::white);
                    waveTrigIn.draw(0, width  - PATTERN_GAP, Qt::white);
                    break;
                }
            }
            else
            {
                waveTrigIn.draw(0, width, Qt::white);
            }

            if ((e->trigIn) && (insertDark))
            {
                if (m_invertTrigOut1)
                {
                    waveTrigOut1.draw(1, PATTERN_GAP, Qt::white);
                    waveTrigOut1.draw(0, imgWin_width, Qt::white);
                    waveTrigOut1.draw(1, PATTERN_GAP, Qt::white);
                }
                else
                {
                    waveTrigOut1.draw(0, PATTERN_GAP, Qt::white);
                    waveTrigOut1.draw(1, imgWin_width, Qt::white);
                    waveTrigOut1.draw(0, PATTERN_GAP, Qt::white);
                }
            }
            else if (e->trigIn)
            {
                if (m_invertTrigOut1)
                {
                    waveTrigOut1.draw(1, PATTERN_GAP, Qt::white);
                    waveTrigOut1.draw(0, (width - PATTERN_GAP), Qt::white);
                }
                else
                {
                    waveTrigOut1.draw(0, PATTERN_GAP, Qt::white);
                    waveTrigOut1.draw(1, (width - PATTERN_GAP), Qt::white);
                }
            }
            else if (insertDark)
            {
                if (m_invertTrigOut1)
                {
                    waveTrigOut1.draw(0, imgWin_width, Qt::white);
                    waveTrigOut1.draw(1, PATTERN_GAP, Qt::white);
                }
                else
                {
                    waveTrigOut1.draw(1, imgWin_width, Qt::white);
                    waveTrigOut1.draw(0, PATTERN_GAP, Qt::white);
                }
            }
            else
            {
                if (m_invertTrigOut1)
                    waveTrigOut1.draw(0, width, Qt::white);
                else
                    waveTrigOut1.draw(1, width, Qt::white);
            }

            if (e->trigOut2)
            {
                if (m_invertTrigOut2)
                {
                    if ((e->trigIn))
                    {
                        waveTrigOut2.draw(1, PATTERN_GAP, Qt::white);
                        waveTrigOut2.draw(0, PATTERN_GAP, Qt::white);
                        waveTrigOut2.draw(1, (width - PATTERN_GAP - PATTERN_GAP) , Qt::white);
                    }
                    else
                    {
                        waveTrigOut2.draw(0, PATTERN_GAP, Qt::white);
                        waveTrigOut2.draw(1, width - PATTERN_GAP , Qt::white);
                    }
                }
                else
                {
                    if ((e->trigIn))
                    {
                        waveTrigOut2.draw(0, PATTERN_GAP, Qt::white);
                        waveTrigOut2.draw(1, PATTERN_GAP, Qt::white);
                        waveTrigOut2.draw(0, (width - PATTERN_GAP -PATTERN_GAP), Qt::white);
                    }
                    else
                    {
                        waveTrigOut2.draw(1, PATTERN_GAP, Qt::white);
                        waveTrigOut2.draw(0, width - PATTERN_GAP, Qt::white);
                    }
                }
            }
            else
            {
                waveTrigOut2.draw(0, width, Qt::white);
            }

            x += width;
        }
    }
}

/**
 * Select a set of patterns on the pattern preview window
 * @param type Type of slection
 * @param index Index of the selected pattern
 */
void WaveFormWindow::select(SelectType type, int index)
{
    int newSelection = -1;
    int start;
    int end;

    switch(type)
    {
    case SELECT_CONTINUE:
        if(m_selectedPattern < 0)
        {
            start = index;
            end = index;
        }
        else if(m_selectedPattern < index)
        {
            start = m_selectedPattern;
            end = index;
        }
        else
        {
            start = index;
            end = m_selectedPattern;
        }

        for(int i = 0; i < m_elements.size(); i++)
        {
            if(i >= start && i <= end)
                m_elements[i].selected = true;
            else
                m_elements[i].selected = false;
        }
        newSelection = index;
        break;

    case SELECT_MULTIPLE:
        m_elements[index].selected = !m_elements[index].selected;

        if(m_elements[index].selected)
            newSelection = index;
        else
            newSelection = findNextSelection();
        break;

    case SELECT_SINGLE:
        for(int i = 0; i < m_elements.size(); i++)
        {
            if(i == index)
            {
                m_elements[i].selected = true;
                newSelection = i;
            }
            else
                m_elements[i].selected = false;
        }
        break;

    case SELECT_ALL:
        for(int i = 0; i < m_elements.size(); i++)
        {
            m_elements[i].selected = true;
        }
        newSelection = m_selectedPattern >= 0 ? m_selectedPattern : 0;
        break;

    case SELECT_NONE:
    default:
        for(int i = 0; i < m_elements.size(); i++)
        {
            m_elements[i].selected = false;
        }
        newSelection = -1;
    }

    setSelection(newSelection);

    update();
}

/**
 * Generates the selection change event
 * @param index Pattern index of the selection
 */
void WaveFormWindow::setSelection(int index)
{

    m_selectedPattern = index;
    emit selectionChange(index, m_elements);
}
/**
 * Find the next selected pattern
 *
 * @return Next seletec index
 */
int WaveFormWindow::findNextSelection(void)
{
    if(m_selectedPattern < 0)
        return -1;

    int newSelection = -1;
    int minDiff = m_elements.size();

    for(int i = 0; i < m_elements.size(); i++)
    {
        if(m_elements[i].selected)
        {
            int diff = m_selectedPattern - i;
            if(diff < 0)
                diff = -diff;
            if(diff < minDiff)
            {
                minDiff = diff;
                newSelection = i;
            }
        }
    }
    return newSelection;
}

/**
 * Converts mouse postition to pattern index
 * @param x X Coordinate of the mouse
 * @param y Y Coordinate of the mouse
 * @return  Index of the pattern
 */
int WaveFormWindow::positionToPattern(int x, int y)
{
    int total = 0;

    if(y >= height())
        return -1;

    for(int i = 0;  i < m_elements.size(); i++)
    {
        total = total + (PTN_IMG_WIDTH * m_scale) + PATTERN_GAP + PATTERN_GAP;
        if((x < total) && (y <= (PTN_NUM_WIN_HEIGHT + PTN_IMG_HEIGHT)))
        {
            return i;
        }
    }
    return -1;
}

/**
 * Event handler for the mouse. This is called by the Qt system
 * @param event Mouse event
 */
void WaveFormWindow::mousePressEvent(QMouseEvent *event)
{
    int index = positionToPattern(event->x(), event->y());
    int modifier = QApplication::keyboardModifiers();

    if(index < 0)
        select(SELECT_NONE, index);
    else if(modifier & Qt::ShiftModifier)
        select(SELECT_CONTINUE, index);
    else if(modifier & Qt::ControlModifier)
        select(SELECT_MULTIPLE, index);
    else
        select(SELECT_SINGLE, index);
    return;
}

/**
 * Set the zoom scale for the pattern preview display
 *
 * @param zoom Zoom scale in 1-100
 */
void WaveFormWindow::setZoom(int zoom)
{
    m_scale = 0.001 * zoom;
    return;
}

/**
 * Sets the pattern image resolution
 *
 * @param width Width in pixels
 * @param height Height in pixels
 */
void WaveFormWindow::setPatternSize(int width, int height)
{
    if(m_ptnWidth == width && m_ptnHeight == height)
        return;

    m_ptnWidth = width;
    m_ptnHeight = height;

    width = DIV_ROUND(width * PTN_IMG_HEIGHT, height);

    m_accessCounter = 0;

    for(int i = 0; i < MAX_PTN_CACHE; i++)
    {
        delete m_ptnImageCache[i].img;

        m_ptnImageCache[i].img = new QImage(width, PTN_IMG_HEIGHT, QImage::Format_Indexed8);
        m_ptnImageCache[i].access = 0;
        m_ptnImageCache[i].bitDepth = 0;
        m_ptnImageCache[i].fileName = "";

        m_ptnImageCache[i].img->setColorCount(256);

        for(int c = 0; c < 256; c++)
        {
            m_ptnImageCache[i].img->setColor(c, qRgb(c, c, c));
        }
    }
}

/**
 * Loads the pattern preview image according to the bit depth
 *
 * @param name Name of the pattern image file
 * @param bitDepth Bitdepth to display
 * @return Preview size image of the pttern
 */
QImage *WaveFormWindow::loadImage(QString name, int bitDepth)
{
    int cacheIndex = 0;

    if(bitDepth > 8 || bitDepth < 0 || name.isEmpty())
        return NULL;

    m_accessCounter++;

    for(int i = 0; i < MAX_PTN_CACHE; i++)
    {
        if(m_ptnImageCache[i].fileName == name)
        {
            if(m_ptnImageCache[i].bitDepth == bitDepth)
            {
                m_ptnImageCache[cacheIndex].access = m_accessCounter;
                return m_ptnImageCache[i].img;
            }
            cacheIndex = i;
            break;
        }
        if(m_ptnImageCache[i].access < m_ptnImageCache[cacheIndex].access)
        {
            cacheIndex = i;
        }
    }

    QImage img(name);
    if(img.isNull())
        return NULL;

    int scaleHeight = DIV_ROUND(PTN_IMG_HEIGHT * img.height(),m_ptnHeight);
    QImage scaledImage = img.scaledToHeight(scaleHeight);

    quantize(scaledImage, bitDepth, m_ptnImageCache[cacheIndex].img);

    m_ptnImageCache[cacheIndex].bitDepth = bitDepth;
    m_ptnImageCache[cacheIndex].fileName = name;
    m_ptnImageCache[cacheIndex].access = m_accessCounter;

    return  m_ptnImageCache[cacheIndex].img;
}

/**
 * Quantize the given QImage to the given bit depth
 * @param src Source image
 * @param bitDepth Destination bit depth
 * @param dst Destination image
 * @return  Quantized image
 */
int WaveFormWindow::quantize(QImage const &src, int bitDepth, QImage *dst)
{
    uint08 mask = GEN_BIT_MASK(8-bitDepth,bitDepth);
    uint08 orMask = 0xFFUL >> bitDepth;

    int minHeight = MIN(dst->height(),src.height());
    int minWidth = MIN(dst->width(),src.width());

    int y;
    for(y = 0; y < minHeight; y++)
    {
        uint08 *ptr = (uint08 *)dst->scanLine(y);
        int x;
        for(x = 0; x < minWidth; x++)
        {
            QRgb pix = src.pixel(x, y);
            uint08 gray = pix | (pix >> 8) | (pix >> 16);
            gray &= mask;
            if(gray)
                gray |= orMask;
            *ptr++ = gray;
        }
        for(;x < dst->width(); x++)
        {
            *ptr++ = 0;
        }
    }

    for(;y < dst->height(); y++)
    {
        memset(dst->scanLine(y), 0, dst->width());
    }

    return SUCCESS;
}
