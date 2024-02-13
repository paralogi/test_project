#!/usr/bin/env python
# -*- coding: utf-8 -*- 

from PyQt5 import QtCore, QtGui, QtWidgets

__all__ = ['RangeSliderWidget']

class RangeSliderWidget(QtWidgets.QWidget):
    regionChanged = QtCore.pyqtSignal(float, float)

    def __init__(self, single=True, caption='', unitfactor=1, unitdigits=0, parent=None):
        super(RangeSliderWidget, self).__init__(parent)
        self.setObjectName("RangeSliderWidget")
        self.setMouseTracking(True)
        self._epsilon = 1e-10
        self._offset = 2
        self._handle = 4
        self._pressed = None
        self._hover = None
        self._cursor = None
        self._single = single
        self._caption = caption
        self._unitfactor = unitfactor
        self._unitdigits = unitdigits
        self._step = 1
        self._range = [0, 99]
        self._rangeChanged()
        self._window = self._range
        self._windowChanged()
        self._region = self._range
        self._foreground1 = QtGui.QColor('#ccc')
        self._foreground2 = QtGui.QColor('#fff')
        self._background1 = QtGui.QLinearGradient(self.geometry().topLeft(), self.geometry().bottomLeft())
        self._background1.setStops([(0, QtGui.QColor('#222')), (1, QtGui.QColor('#333'))])
        self._background2 = QtGui.QLinearGradient(self.geometry().topLeft(), self.geometry().bottomLeft())
        self._background2.setStops([(0, QtGui.QColor('#282')), (1, QtGui.QColor('#393'))])
        self._highlight = [QtGui.QColor('#5ca'), QtGui.QColor('#ca5')]
    
    @property
    def step(self):
        return self._step

    @step.setter
    def step(self, value):
        self._step = value
        self._rangeChanged()
        self._windowChanged()
        self.update()
    
    @property
    def range(self):
        return self._range

    @range.setter
    def range(self, value):
        self._range = value
        self._rangeChanged()
        self._window = self._range
        self._windowChanged()
        self.update()
    
    @property
    def region(self):
        return self._region

    @region.setter
    def region(self, value):
        self._region = value
        self.update()

    @property
    def first(self):
        return self._region[0]

    @first.setter
    def first(self, value):
        self._region[0] = value
        self.update()

    @property
    def second(self):
        return self._region[1]

    @second.setter
    def second(self, value):
        self._region[1] = value
        self.update()

    def _min(self, value, bound):
        if self._step > 0:
            return min(value, bound)
        else:
            return max(value, bound)

    def _max(self, value, bound):
        if self._step > 0:
            return max(value, bound)
        else:
            return min(value, bound)
        
    def _leq(self, value, bound):
        if self._step > 0:
            return value <= bound
        else:
            return value >= bound
        
    def _round(self, value):
        return round(value * self._unitfactor, self._unitdigits)

    def _position(self, value):
        index = round((value - self._window[0]) / self._step + self._epsilon)
        ratio = max(min(index / self._window_capacity, 1), 0)
        return int(ratio * (self.width() - self._handle))

    def _value(self, position):
        ratio = position / (self.width() - self._handle)
        index = round(ratio * self._window_capacity + self._epsilon)
        return index * self._step + self._window[0]
    
    def _rangeChanged(self):
        self._range_capacity = (self._range[1] - self._range[0]) / self._step
        self._zoom = 0.1 * (self.width() - self._handle) / self._range_capacity
    
    def _windowChanged(self):
        self._window_capacity = (self._window[1] - self._window[0]) / self._step
        self._space = (self.width() - self._handle) / self._window_capacity
    
    def mousePressEvent(self, event):
        changed = False
        btn = event.button()
    
        if btn == QtCore.Qt.LeftButton:
            if self._hover is not None:
                value = self._value(event.x())
                changed |= self._region[self._hover] != value
                self._region[self._hover] = value
                self._pressed = self._hover
                self.update()
                
        elif btn == QtCore.Qt.RightButton:
            if self._space < 10:
                value = self._value(event.x())
                offset = value * (1 - self._zoom)
                self._window = [self._max(self._range[0], offset + self._range[0] * self._zoom),
                                self._min(self._range[1], offset + self._range[1] * self._zoom)]
                self._windowChanged()
                self.update()
                
        if changed:
            self.regionChanged.emit(*self._region)
        
    def mouseReleaseEvent(self, event):
        btn = event.button()
        
        if btn == QtCore.Qt.LeftButton:
            self._pressed = None
            self.update()
            
        elif btn == QtCore.Qt.RightButton:
            self._window = self._range
            self._windowChanged()
            self.update()
     
    def mouseMoveEvent(self, event):
        changed = False
        pos = event.x()
        
        if pos >= 0 and pos <= self.width() - self._handle:
            value = self._value(pos)
            
            if self._pressed is not None:
                value = self._min(value, self._region[1]) if self._pressed == 0 else self._max(value, self._region[0])
                changed = self._region[self._pressed] != value
                self._region[self._pressed] = value

            if not self._single and self._leq(value, (self._region[0] + self._region[1]) / 2):
                self._hover = 0
            elif self._leq(self._region[0], value):
                self._hover = 1
            else:
                self._hover = None
                
            self._cursor = value
            self.update()
                
        if changed:
            self.regionChanged.emit(*self._region)
            
    def leaveEvent(self, event):
        self._hover = None
        self.update()
    
    def resizeEvent(self, event):
        self._zoom = 0.1 * (self.width() - self._handle) / self._range_capacity
        self._space = (self.width() - self._handle) / self._window_capacity
        self.update()
        
    def paintEvent(self, event):
        painter = QtGui.QPainter(self)
        painter.setPen(self._foreground1)
        painter.setFont(self.font())
        
        minstr = str(self._round(self._window[0]))
        maxstr = str(self._round(self._window[1]))
        leftstr = str(self._round(self._max(self._window[0], self._min(self._region[0], self._window[1]))))
        rightstr = str(self._round(self._max(self._window[0], self._min(self._region[1], self._window[1]))))

        minpos = 0
        maxpos = self.width()
        leftpos = self._position(self._region[0])
        rightpos = self._position(self._region[1])
        minsize = painter.fontMetrics().horizontalAdvance(minstr) + 2 * self._offset
        maxsize = painter.fontMetrics().horizontalAdvance(maxstr) + 2 * self._offset
        leftsize = painter.fontMetrics().horizontalAdvance(leftstr) + 2 * self._offset
        rightsize = painter.fontMetrics().horizontalAdvance(rightstr) + 2 * self._offset
        
        headpos = minpos
        bodypos = leftpos
        tailpos = rightpos
        headsize = leftpos - minpos
        bodysize = rightpos - leftpos
        tailsize = maxpos - rightpos

        rectlist = []
        strlist = []

        if headsize > 0:
            rectlist.append((headpos, headsize, self._background1))
        if tailsize > 0:
            rectlist.append((tailpos, tailsize, self._background1))
        if bodysize + self._handle > 0:
            rectlist.append((bodypos, bodysize + self._handle, self._background2))

        if headsize >= minsize:
            strlist.append((minpos, minpos + minsize, minstr))
        if tailsize >= maxsize:
            strlist.append((maxpos - maxsize, maxpos, maxstr))
        if bodysize >= leftsize:
            strlist.append((leftpos, leftpos + leftsize, leftstr))
        elif headsize >= minsize + leftsize:
            strlist.append((leftpos - leftsize, leftpos, leftstr))
        if bodysize >= leftsize + rightsize:
            strlist.append((rightpos - rightsize, rightpos, rightstr))
        elif tailsize >= rightsize + maxsize:
            strlist.append((rightpos, rightpos + rightsize, rightstr))
            
        if self._pressed is not None:
            cursorpos = self._position(self._cursor)
            rectlist.append((cursorpos, self._handle, self._highlight[self._pressed]))
            
        elif self._hover is not None:
            cursorstr = str(self._round(self._cursor))
            cursorpos = self._position(self._cursor)
            cursorsize = painter.fontMetrics().horizontalAdvance(cursorstr) + 2 * self._offset
            rectlist.append((cursorpos, self._handle, self._highlight[self._hover]))
            
            if maxpos - cursorpos >= cursorsize:
                crossref = (cursorpos, cursorpos + cursorsize, cursorstr)
            elif cursorpos - minpos >= cursorsize:
                crossref = (cursorpos - cursorsize, cursorpos, cursorstr)

            crosslist = []
            crosslist.append(crossref)

            for strref in strlist:
                if strref[0] >= crossref[1] or strref[1] <= crossref[0]:
                    crosslist.append(strref)
            
            strlist = crosslist
            
        for rectref in rectlist:
            painter.fillRect(rectref[0], 0, rectref[1], self.height(), rectref[2])
            
        for strref in strlist:
            painter.drawText(strref[0] + self._offset, 0, strref[1] - self._offset, self.height(), 0, strref[2])
            
        if self._space > 4:
            pos = minpos
            while pos < maxpos - 2:
                painter.drawLine(int(pos), 0, int(pos), 2)
                pos += self._space
            
        captionfont = self.font()
        captionfont.setPointSize(self.font().pointSize() + 2)
        painter.setPen(self._foreground2)
        painter.setFont(captionfont)
        painter.drawText(self._offset, self.height() - painter.fontMetrics().height() - self._offset, 
                         self.width() - self._offset, self.fontMetrics().height() + self._offset, 
                         QtCore.Qt.AlignLeft, self._caption)
    
    def minimumSizeHint(self):
        captionfont = self.font()
        captionfont.setPointSize(self.font().pointSize() + 2)
        captionmetrics = QtGui.QFontMetrics(captionfont)
        return QtCore.QSize(
            0, captionmetrics.height() + self.fontMetrics().height() + 2 * self._offset)
        
    def sizeHint(self):
        captionfont = self.font()
        captionfont.setPointSize(self.font().pointSize() + 2)
        captionmetrics = QtGui.QFontMetrics(captionfont)
        return QtCore.QSize(
            max(captionmetrics.horizontalAdvance(self._caption) + 2 * self._offset, 300),
            captionmetrics.height() + self.fontMetrics().height() + 2 * self._offset)

if __name__ == '__main__':
    import sys, math
    qapp = QtWidgets.QApplication(sys.argv)
    slider = RangeSliderWidget(False, 'example', 1, 1)
    slider.step = 0.1
    slider.range = [1, 10]
    slider.region = [3, 7]
    slider.show()    
    qapp.exec_()
