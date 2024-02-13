#!/usr/bin/env python
# -*- coding: utf-8 -*- 

import math
import cmath
import numpy
from PyQt5 import QtCore
from vispy import scene, color

class PolarizationDiagram(scene.visuals.Line):
    def __init__(self, num=36, parent=None):
        self.phase = numpy.exp(-1j * numpy.linspace(0, 2 * math.pi, num))
        colormap = color.Colormap(['g', 'b'])[numpy.linspace(0, 1, num)]
        super(PolarizationDiagram, self).__init__(width=2, color=colormap, parent=parent)
        
    def updateData(self, horizontal, vertical):
        self.set_data(pos=numpy.column_stack((numpy.real(horizontal * self.phase), numpy.real(vertical * self.phase))))
      
class PolarizationView(scene.SceneCanvas):
    def __init__(self, parent):
        super(PolarizationView, self).__init__(keys='interactive')
        self.create_native()
        self.native.setParent(parent)
        self.unfreeze()
        self.parent = parent
        self.ph = None
        self.pv = None
        self.view = self.central_widget.add_view()
        self.view.camera = scene.PanZoomCamera(rect=[-1.1, -1.1, 2.2, 2.2], aspect=1)
        self.diagram = PolarizationDiagram(parent=self.view.scene)
        self.grid = scene.visuals.GridLines(parent=self.view.scene)
        self.grid.set_gl_state(depth_test=False)
        self.axx = scene.visuals.Axis(pos=[[-1, 0], [1, 0]], domain=(-1., 1.), axis_width=1, tick_direction=(1, 1), parent=self.view.scene)
        self.axx.set_gl_state(depth_test=False)
        self.axy = scene.visuals.Axis(pos=[[0, -1], [0, 1]], domain=(-1., 1.), axis_width=1, tick_direction=(1, 1), parent=self.view.scene)
        self.axy.set_gl_state(depth_test=False)
        self.freeze()
        
    def updatePolarization(self, theta, phi, sigma):
        r = cmath.rect(1, math.radians(theta))
        self.ph = cmath.rect(r.real * sigma, 0)
        self.pv = cmath.rect(r.imag, math.radians(phi))
        
    def updateData(self, angle):
        ex = cmath.rect(1, math.radians(angle))
        ey = ex * 1j
        horizontal = (ex.real * self.ph + ey.real * self.pv)
        vertical = (ex.imag * self.ph + ey.imag * self.pv)
        self.diagram.updateData(horizontal, vertical) 
        
if __name__ == '__main__':
    numpy.set_printoptions(suppress=True, precision=6)
    
    view = PolarizationView(None)
    view.updatePolarization(3/5 * 90, 5/4 * 90, 0.5)
    angle = 0
    
    def timerEvent():
        global angle
        view.updateData(angle)
        angle = math.fmod(angle + 5, 360)
        
    timer = QtCore.QTimer()
    timer.timeout.connect(timerEvent)
    timer.start(100)

    view.show(run=True)
