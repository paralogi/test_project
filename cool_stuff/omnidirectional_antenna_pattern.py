#!/usr/bin/env python
# -*- coding: utf-8 -*- 

import math
import cmath
import numpy
from PyQt5 import QtCore
from PIL import Image
from io import BytesIO
from vispy import scene, color, io

def dipoleAmplitude(phase):
    wave_length = 300 / 70
    unit_length = 1.5
    
    if abs(phase) == 1:
        return 0
    
    phase_factor = math.pi / wave_length * unit_length
    return (numpy.cos(phase * phase_factor) - math.cos(phase_factor)) / numpy.sqrt(1 - phase * phase) / (1 - math.cos(phase_factor))

class LineDiagram(scene.visuals.Line):
    def __init__(self, colors, parent=None):
        super(LineDiagram, self).__init__(width=2, color=colors, parent=parent)
        
    def updateData(self, points):
        self.set_data(pos=numpy.column_stack((numpy.real(points), numpy.imag(points))))
      
class PlotView(scene.SceneCanvas):
    def __init__(self, parent):
        super(PlotView, self).__init__(keys='interactive')
        self.create_native()
        self.native.setParent(parent)
        self.unfreeze()
        self.parent = parent
        self.decart_mesh = None
        self.view = self.central_widget.add_view()
        self.view.camera = scene.PanZoomCamera(rect=[-1.1, -1.1, 2.2, 2.2], aspect=1)
        self.diagram2 = LineDiagram('g', parent=self.view.scene)  
        self.diagram1 = LineDiagram('b', parent=self.view.scene)
        self.grid = scene.visuals.GridLines(parent=self.view.scene)
        self.grid.set_gl_state(depth_test=False)
        self.axx = scene.visuals.Axis(pos=[[-1, 0], [1, 0]], domain=(-1., 1.), axis_width=1, tick_direction=(1, 1), parent=self.view.scene)
        self.axy = scene.visuals.Axis(pos=[[0, -1], [0, 1]], domain=(-1., 1.), axis_width=1, tick_direction=(1, 1), parent=self.view.scene)
        self.freeze()
        
    def updateData(self, azimuth, angle=120):
        sector = (azimuth + 45) // 90
        phi = cmath.rect(1, math.radians(azimuth))
        phi0 = cmath.rect(1, math.radians(0 * angle))
        phi1 = cmath.rect(1, math.radians(1 * angle))
        phi2 = cmath.rect(1, math.radians(2 * angle))
        
        f0 = dipoleAmplitude((numpy.conj(phi0) * phi).real)
        f1 = dipoleAmplitude((numpy.conj(phi1) * phi).real)
        f2 = dipoleAmplitude((numpy.conj(phi2) * phi).real)

        us = (f0 * phi0 + f1 * phi1 + f2 * phi2)
        ud = (f0 * numpy.conj(phi0) + f1 * numpy.conj(phi1) + f2 * numpy.conj(phi2))
        psi = sector * (math.pi / 2) - cmath.phase(us / ud) / 4
        delta = math.degrees(psi) - azimuth
                
        self.diagram1.updateData(numpy.array([0j, phi]))
        self.diagram2.updateData(numpy.array([0j, cmath.rect(1, psi)]))
        
if __name__ == '__main__':
    numpy.set_printoptions(suppress=True, precision=1)
    
    view = PlotView(None)
    images = []
    azimuth = 0
    
    def timerEvent():
        global azimuth
        view.updateData(azimuth)
        images.append(Image.open(BytesIO(io._make_png(view.render()))))
        azimuth = math.fmod(azimuth + 1, 360)
        
    timer = QtCore.QTimer()
    timer.timeout.connect(timerEvent)
    timer.start(100)

    view.show(run=True)
    images[0].save(fp='azimuth.gif', format='GIF', append_images=images[1:], save_all=True, optimize=False, duration=100, loop=0)
