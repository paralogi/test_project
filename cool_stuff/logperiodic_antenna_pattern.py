#!/usr/bin/env python
# -*- coding: utf-8 -*- 

import math
import cmath
import numpy
import scipy
from scipy import integrate
from PyQt5 import QtCore
from vispy import scene, geometry, color
from vispy.visuals import filters

class LPDA:
    def __init__(self, tau, count, lr, l0, d0, h0, a0):
        self.N = numpy.arange(count)
        self.Y = numpy.zeros((count, count), dtype=complex)
        self.Z = numpy.zeros((count, count), dtype=complex)
        self.I = numpy.zeros((count), dtype=complex)
        self.DI = numpy.zeros((count), dtype=complex)
        self.D0 = numpy.zeros((count), dtype=complex)
        self.l = numpy.zeros((count), dtype=float)
        self.h = numpy.zeros((count), dtype=float)
        self.kl = numpy.zeros((count), dtype=float)
        self.kh = numpy.zeros((count), dtype=float)
        self.ch = numpy.zeros((count), dtype=float)
        self.sh = numpy.zeros((count), dtype=float)
        
        self.impendance = 0j
        self.wave_length = 0
        self.phase_number = 0
        self.a0 = a0
        self.lr = lr
        self.l[0] = l0
        self.h[0] = h0
        dl = d0
            
        for n in self.N[1:]:
            self.l[n] = self.l[n - 1] - dl
            self.h[n] = self.h[n - 1] * tau
            dl *= tau
        
        # for n in self.N[1:]:
        #     print(n, self.l[n] + 0.755, self.h[n] * 2 + 0.1)
        
    def update(self, wave_length):
        self.phase_number = 2 * math.pi / wave_length
        
        w0 = 120 * math.pi
        wn = 0.5 * w0 / math.pi
        ka = self.phase_number * self.a0
        kr = self.phase_number * self.lr
        self.kl = self.phase_number * self.l
        self.kh = self.phase_number * self.h
        self.ch = numpy.cos(self.kh)
        self.sh = numpy.sin(self.kh)
        
        self.Y[0, 0] = 1 / math.tan(kr - self.kl[0]) + 1 / math.tan(self.kl[0] - self.kl[1])
        self.Y[-1, -1] = 1 / math.tan(self.kl[-2] - self.kl[-1])
        self.Y[self.N[1:-1], self.N[1:-1]] = 1 / numpy.tan(self.kl[:-2] - self.kl[1:-1]) + 1 / numpy.tan(self.kl[1:-1] - self.kl[2:])
        self.Y[self.N[:-1], self.N[:-1] + 1] = 1 / numpy.sin(self.kl[:-1] - self.kl[1:])
        self.Y[self.N[:-1] + 1, self.N[:-1]] = self.Y[self.N[:-1], self.N[:-1] + 1]
        
        self.Z = numpy.linalg.inv(-1j * self.Y / w0)
        
        def fi(x, kh):
            return numpy.sin(kh - numpy.fabs(x))
        
        def fpi(x, y, kh, ch):
            r = numpy.sqrt((x - kh)**2 + y) 
            r0 = numpy.sqrt(x**2 + y)
            return numpy.exp(-1j * r) / r - ch * numpy.exp(-1j * r0) / r0
        
        def cquad(f, a, b):
            r = scipy.integrate.quad(lambda x: numpy.real(f(x)), a, b)
            i = scipy.integrate.quad(lambda x: numpy.imag(f(x)), a, b)
            return r[0] + 1j * i[0]
        
        A = numpy.empty_like(self.Z)
        
        for n in self.N:
            y = (self.kl[n] - self.kl)**2
            y[n] = ka**2
    
            for m in self.N:
                f = lambda x: fi(x, self.kh[m]) * fpi(x, y[m], self.kh[n], self.ch[n])
                A[n, m] = cquad(f, -self.kh[m], self.kh[m]) / self.sh[m]
        
        A = -A + 1j * self.Z * self.sh.reshape(-1, 1) / wn
        B = 1j * self.Z[self.N, -1] * self.sh / wn
        self.I = numpy.linalg.solve(A, B)
        self.DI = self.I * wn / self.sh
        self.D0 = self.DI * (1 - self.ch)
        
        self.impendance = self.Z[-1, -1] - numpy.dot(self.Z[-1], self.I)
        print('lambda =', wave_length, 'impendance =', self.impendance)
        
        return self
        
    def dipolePattern(self, phase, index):
        phase = numpy.array(phase)
        phase_mask = numpy.fabs(phase[0]) != 1
        phase_masked = phase[0, phase_mask]
    
        amplitude = numpy.zeros_like(phase[0], dtype=float)
        amplitude[phase_mask] = (numpy.cos(phase_masked * self.kh[index]) - self.ch[index]) / numpy.sqrt(1 - phase_masked**2)
    
        return amplitude + 0j
    
    def pattern(self, phase):    
        pattern = 0j
    
        for n in self.N:
            pattern += self.DI[n] * self.dipolePattern(phase, n) * numpy.exp(-1j * phase[1] * self.kl[n])
    
        return pattern
    
    def directivity(self):
        md = 0j
    
        for n in self.N:
            md += self.D0[n] * numpy.exp(-1j * self.kl[n])
            
        kn = 10 * math.log10((md.real**2 + md.imag**2) / (30 * self.impendance.real))
        return kn
    
# tau = d1 / d0
# n = math.ceil(1 + math.log(f1 / f0 / 0.6, 1 / tau))
# l0 = c / f0 / 2
# s0 = d0 * (tau**n - 1) / (tau - 1)
lpda = LPDA(0.887159533, 12, 3.958, 3.346, 0.514, 2.141, 0.045)
    
class Diagram(scene.visuals.Mesh):
    def __init__(self, parent=None):
        super(Diagram, self).__init__(parent=parent)
        self.unfreeze()
        self.light_dir = None
        self.frame_mode = None
        self.visual_data = geometry.MeshData()
        self.shading_filter = filters.ShadingFilter(shading='smooth')
        self.wireframe_filter = filters.WireframeFilter(enabled=False, color=(*color.Color('green').rgb, 0.5), width=1)
        self.freeze()
        
    def updateShading(self, view, light_dir=(0, 1, 0, 0)):
        view_light_dir = view.camera.transform.imap(light_dir)
        if numpy.equal(self.light_dir, view_light_dir).all():
            return
        
        self.light_dir = view_light_dir
        self.shading_filter.light_dir = light_dir[:3]
        self.update()

        @view.scene.transform.changed.connect
        def on_transform_change(event):
            self.shading_filter.light_dir = view.camera.transform.map(self.light_dir)[:3]
            
    def updateWireframe(self, frame_mode=0):
        if self.frame_mode == frame_mode:
            return
        
        self.frame_mode = frame_mode
        self.wireframe_filter.enabled = (frame_mode != 0)
        self.wireframe_filter.wireframe_only = (frame_mode == 1)
        self.update()

    def updateView(self, target_mesh, delta_mesh):
        global lpda
        # radiation_pattern = numpy.absolute(lpda.pattern(target_mesh) / lpda.pattern([0, 1, 0]))
        # print(10 * math.log10(4 * math.pi / numpy.sum(delta_mesh * radiation_pattern**2)), lpda.directivity())
        
        radiation_pattern = numpy.absolute(lpda.pattern(target_mesh))
        radiation_pattern = radiation_pattern / numpy.max(radiation_pattern)
        diagram_mesh = radiation_pattern * target_mesh
        vertices, indices = geometry.create_grid_mesh(diagram_mesh[0], diagram_mesh[1], diagram_mesh[2])
        normals = geometry._calculate_normals(vertices, indices)
        colors = color.get_colormap('viridis').map(radiation_pattern)
        self.visual_data.set_vertices(vertices)
        self.visual_data.set_faces(indices)
        self.visual_data._vertex_normals = normals
        self.visual_data.set_vertex_colors(colors)
        self.updateData()

    def updateData(self):
        self.set_data(meshdata=self.visual_data)
        if self.shading_filter.attached == False:
            self.attach(self.shading_filter)
        if self.wireframe_filter.attached == False:
            self.attach(self.wireframe_filter)

class PlotView(scene.SceneCanvas):
    def __init__(self, parent):
        super(PlotView, self).__init__(keys='interactive')
        self.create_native()
        self.native.setParent(parent)
        self.unfreeze()
        self.parent = parent
        self.light_dir = (0, 1, 0, 0)
        self.frame_mode = 0
        self.target_mesh = None
        self.delta_mesh = None
        self.view = self.central_widget.add_view()
        self.view.camera = scene.TurntableCamera(elevation=30, azimuth=140, roll=0, distance=3)
        self.grid = scene.visuals.GridLines(parent=self.view.scene)
        self.axes = scene.visuals.Line(
            pos=1.1 * numpy.array([[-1, 0, 0], [1, 0, 0], [0, -1, 0], [0, 1, 0], [0, 0, -1], [0, 0, 1]]), 
            color=numpy.array([[1, 1, 0, 1], [1, 0, 0, 1], [1, 1, 0, 1], [0, 1, 0, 1], [1, 1, 0, 1], [0, 0, 1, 1]]),
            width=2, connect='segments', method='gl', parent=self.view.scene)
        self.diagram = Diagram(parent=self.view.scene)
        self.freeze()
        
    def updateMesh(self, resolution=180):
        psi, phi = numpy.meshgrid(
            numpy.linspace(-math.pi, math.pi, 2 * resolution), 
            numpy.linspace(-math.pi / 2, math.pi / 2, resolution))
        
        self.target_mesh = numpy.array([numpy.sin(psi) * numpy.cos(phi), numpy.cos(psi) * numpy.cos(phi), numpy.sin(phi)])
        self.delta_mesh = numpy.cos(phi) * (math.pi / resolution)**2
        
    def updateData(self):
        self.diagram.updateView(self.target_mesh, self.delta_mesh)
        self.diagram.updateShading(self.view, self.light_dir)
        
    def on_key_press(self, event):
        if self.target_mesh is None:
            return
        if event.key == 's' and 'Control' in event.modifiers:
            self.diagram.updateShading(self.view, self.light_dir)
        if event.key == 'w' and 'Control' in event.modifiers:
            self.frame_mode = self.frame_mode + 1 if self.frame_mode < 2 else 0
            self.diagram.updateWireframe(self.frame_mode)
        
if __name__ == '__main__':
    numpy.set_printoptions(suppress=True, precision=6)
    
    antenna = lpda.update(300 / 70)
    # print(antenna.pattern([0, 1, 0]))
    # print(antenna.directivity())
    
    view = PlotView(None)
    view.updateMesh(60)
    # view.updateData()
    # view.show(run=True)
    # exit(0)
    
    freq0 = 3
    freq1 = 300
    freqs = numpy.array([*range(freq0, freq1, 1), *range(freq1, freq0, -1)], dtype=float)
    iter = 0
    
    def timerEvent():
        global freqs, iter, lpda
        lpda.update(300 / freqs[iter % len(freqs)])
        view.updateData()
        iter += 3
    
    timer = QtCore.QTimer()
    timer.timeout.connect(timerEvent)
    timer.start(30)

    view.show(run=True)
