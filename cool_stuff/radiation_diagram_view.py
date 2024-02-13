# -*- coding: utf-8 -*- 

import sys
import copy
import math
import cmath
import numpy
import scipy.special

from PyQt5 import QtCore, QtWidgets, QtGui
from vispy import app, scene, plot, geometry, color
from vispy.visuals import filters
from range_slider_widget import RangeSliderWidget
from PyQt5.Qt import QWidget
        
def Normals(mesh):
    i, j = numpy.unravel_index(numpy.arange(mesh[0].size), shape=mesh[0].shape)
    vertices = mesh.transpose((1, 2, 0))
    sides = numpy.empty((4, vertices.shape[0] * vertices.shape[1], vertices.shape[-1]), dtype=float)
    sides[0] = vertices[numpy.fmax(i - 1, 0), j]
    sides[1] = vertices[i, numpy.fmin(j + 1, mesh[0].shape[1] - 1)]
    sides[2] = vertices[numpy.fmin(i + 1, mesh[0].shape[0] - 1), j]
    sides[3] = vertices[i, numpy.fmax(j - 1, 0)]
    sides -= vertices[i, j]
    product = numpy.cross(sides[0], sides[-1])
    product += numpy.cross(sides[1], sides[0])
    product += numpy.cross(sides[2], sides[1])
    product += numpy.cross(sides[3], sides[2])
    modulus = numpy.sqrt(numpy.sum(product**2, axis=-1))
    normals = (product / modulus[:, None]).reshape(vertices.shape)
    return normals

def MirrorDiagram(phase, phase_factor):
    return (phase > 0) * numpy.sin(phase * phase_factor) / numpy.sin(phase_factor)

def RepulseDiagram(phase, phase_factor, phase_shift):
    return numpy.cos(phase * phase_factor - phase_shift) / numpy.cos(phase_factor - phase_shift)

def DipoleDiagram(phase, phase_factor, phase_moment):
    return (numpy.cos(phase * phase_factor) - phase_moment) / numpy.sqrt(1 - phase * phase) / (1 - phase_moment)

def PairDiagram(phase, phase_factor):
    return numpy.cos(phase * phase_factor)

def GridDiagram(phase, phase_factor, phase_moment):
#     phaseiter = (numpy.arange(phase_moment) - 0.5 * (phase_moment - 1)).reshape(-1, 1)
#     phasesum = numpy.sum(numpy.exp(2j * phase * phase_factor * phaseiter), axis=0)
#     return numpy.abs(phasesum) / phase_moment
    if phase_moment == 2:
        return CommonLogic.pairDiagram(phase, phase_factor)
    return numpy.sin(phase * phase_factor * phase_moment) / numpy.sin(phase * phase_factor) / phase_moment

def SurokDiagram(phase, phase_factor, phase_moment, angle=120):
    phi0 = numpy.full_like(phase, cmath.rect(1, math.radians(0 * angle)))
    phi1 = numpy.full_like(phase, cmath.rect(1, math.radians(1 * angle)))
    phi2 = numpy.full_like(phase, cmath.rect(1, math.radians(2 * angle)))
    phase0 = (numpy.conj(phi0) * phase).real
    phase1 = (numpy.conj(phi1) * phase).real
    phase2 = (numpy.conj(phi2) * phase).real
    mask0 = numpy.fabs(phase0) < 1
    mask1 = numpy.fabs(phase1) < 1
    mask2 = numpy.fabs(phase2) < 1
    phase0 = phase0[mask0]
    phase1 = phase1[mask1]
    phase2 = phase2[mask2]
    pattern0 = numpy.zeros_like(mask0, dtype=float)
    pattern1 = numpy.zeros_like(mask1, dtype=float)
    pattern2 = numpy.zeros_like(mask2, dtype=float)
    pattern0[mask0] = CommonLogic.dipoleDiagram(phase0, phase_factor, phase_moment)
    pattern1[mask1] = CommonLogic.dipoleDiagram(phase1, phase_factor, phase_moment)
    pattern2[mask2] = CommonLogic.dipoleDiagram(phase2, phase_factor, phase_moment)
    pattern = pattern0 + pattern1 + pattern2
    return pattern / 2

def ReleighDistrib():
    fig = plot.Fig(show=False)
    phi = numpy.linspace(0, math.pi, 180)
    cosphi = numpy.cos(phi)
    sinphi = numpy.sin(phi)
    
    wavelength = 50
    phasefactor = 2 * math.pi / wavelength
    colors = color.Colormap(['r', 'b'])[numpy.linspace(0., 1., 10)]
    for i, deviation in numpy.ndenumerate(numpy.linspace(1, 10, 10)):
        sigma = phasefactor * cosphi * deviation
        specularity = numpy.exp(-2 * sigma**2)
        fig[0, 0].plot((specularity * cosphi, specularity * sinphi), color=colors[i[0]])
        
    fig[0, 0].plot((cosphi, sinphi), color='black')
    
    grid = scene.visuals.GridLines(color=(0, 0, 0, 0.5))
    grid.set_gl_state('translucent')
    fig[0, 0].view.add(grid)
    fig.show(run=True)
    
def SquarePlateDiagram():
    fig = plot.Fig(show=False)
    numpy.set_printoptions(precision=6, suppress=True)
        
    wave_length = 30
    plane_length = 30
    phase_number = 2 * math.pi / wave_length
    phase_factor = phase_number * plane_length
    
    psi0, phi0 = (math.radians(0) - math.pi, math.radians(90))
    cosax0 = numpy.sin(psi0) * numpy.cos(phi0)
    cosay0 = numpy.cos(psi0) * numpy.cos(phi0)
    cosaz0 = numpy.sin(phi0)
    
    psi, phi = numpy.meshgrid(
        numpy.radians(numpy.linspace(-180, 180, 360)), 
        numpy.radians(numpy.linspace(-90, 90, 180)))
    cosax = numpy.sin(psi) * numpy.cos(phi)
    cosay = numpy.cos(psi) * numpy.cos(phi)
    cosaz = numpy.sin(phi)
    
    incident = numpy.array([cosax0, cosay0, cosaz0])
    incident = incident / numpy.linalg.norm(incident)
    
    normal = numpy.array([math.tan(math.radians(0)), math.tan(math.radians(0)), 1])
    normal = normal / numpy.linalg.norm(normal)
    
    base1 = numpy.cross(normal, incident)
    base1 = base1 / numpy.linalg.norm(base1) 
    
    base2 = numpy.cross(normal, base1)
    base2 = base2 / numpy.linalg.norm(base2) 
        
    scattering = numpy.array([cosax, cosay, cosaz]).transpose((1, 2, 0))
    
    phase1 = numpy.dot(scattering, base1)
    mask1 = phase1 != 0
    phase1 = phase1[mask1]
    
    phase2 = numpy.dot(scattering, base2) - math.sqrt(1 - numpy.dot(incident, normal)**2)
    mask2 = phase2 != 0
    phase2 = phase2[mask2]

    directivity = numpy.ones(shape=scattering.shape[:2], dtype=float)
    directivity[mask1] *= numpy.sin(phase_factor * phase1) / (phase_factor * phase1)
    directivity[mask2] *= numpy.sin(phase_factor * phase2) / (phase_factor * phase2)
    # directivity[mask1] *= 2 * scipy.special.jv(1, phase_factor * phase1) / (phase_factor * phase1)
    # directivity[mask2] *= 2 * scipy.special.jv(1, phase_factor * phase2) / (phase_factor * phase2)
    directivity = numpy.abs(directivity) * numpy.maximum(0.001, numpy.dot(scattering, normal))#(numpy.dot(scattering, normal) >= 0))

    mesh = directivity * numpy.array([cosax, cosay, cosaz])
    vertices, faces = geometry.create_grid_mesh(*mesh)
    
    meshdata = geometry.MeshData()
    meshdata.set_vertices(vertices)
    meshdata.set_faces(faces)
    meshdata._vertex_normals = Normals(mesh)
    meshdata.set_vertex_colors(color.get_colormap('viridis').map(directivity))
    fig[0, 0].mesh(meshdata=meshdata, shading=None)
    
    fig[0, 0].view.add(scene.visuals.Line(
        pos=numpy.array([[-1, 0, 0], [1, 0, 0], [0, -1, 0], [0, 1, 0], [0, 0, -1], [0, 0, 1], [0, 0, 0], normal, [0, 0, 0], incident]), 
        color=numpy.array([[1, 1, 0, 1], [1, 0, 0, 1], [1, 1, 0, 1], [0, 1, 0, 1], [1, 1, 0, 1], [0, 0, 1, 1], [0, 0, 0, 1], [0, 0, 0, 1], [0, 1, 1, 1], [0, 1, 1, 1]]),
        width=2, connect='segments', method='gl'))
    
    grid = scene.visuals.GridLines(color=(0, 0, 0, 0.5))
    grid.set_gl_state('translucent')
    fig[0, 0].view.add(grid)
    fig.show(run=True)
    
def RepulseDiagram():
    fig = plot.Fig(show=False)
    phi = numpy.linspace(0, 2 * math.pi, 360)
    cosphi = numpy.cos(phi)
    sinphi = numpy.sin(phi)
    
    wavelength = 6
    repulselength = 1.3
    channellength = 1.5
    phasenumber = 2 * math.pi / wavelength
    phasefactor = phasenumber * repulselength
    phaseshift = phasenumber * channellength
    phasedelta = 1
    
    pattern1 = numpy.cos((math.pi / 2 - phasefactor * sinphi) / 2)
    pattern2 = numpy.sqrt(1 + phasedelta**2 + 2 * phasedelta * numpy.cos(phaseshift - phasefactor * sinphi)) / 2

    fig[0, 0].plot((pattern1 * cosphi, pattern1 * sinphi), color='blue')
    fig[0, 0].plot((pattern2 * cosphi, pattern2 * sinphi), color='red')
    fig[0, 0].plot((cosphi, sinphi), color='black')
    
    grid = scene.visuals.GridLines(color=(0, 0, 0, 0.5))
    grid.set_gl_state('translucent')
    fig[0, 0].view.add(grid)
    fig.show(run=True)

def СrossDiagram():
    fig = plot.Fig(show=False)
    phi = numpy.linspace(0, math.pi, 36000)
    cosphi = numpy.cos(phi)
    sinphi = numpy.sin(phi)
    
    wavelength = 6
    unitlength = 2.7 / 2
    phasenumber = 2 * math.pi / wavelength
    phasefactor = phasenumber * unitlength
    phasemoment = math.cos(phasefactor)
    
    phase = cosphi * math.cos(math.radians(45))
    mask = (phase != 1) & (phase != -1)
    phase = phase[mask]
    reciprocal = numpy.sqrt(1 - phase**2)

    pattern = numpy.zeros_like(mask, dtype=float)
    pattern[mask] = (numpy.cos(phasefactor * phase) - phasemoment) / reciprocal
    pattern = numpy.maximum(pattern / (1 - phasemoment), 0)

    fig[0, 0].plot((pattern * cosphi, pattern * sinphi), color='blue')
    fig[0, 0].plot((cosphi, sinphi), color='black')
    
    grid = scene.visuals.GridLines(color=(0, 0, 0, 0.5))
    grid.set_gl_state('translucent')
    fig[0, 0].view.add(grid)
    fig.show(run=True)

if __name__ == '__main__':
    if len(sys.argv) > 1:
        mode = sys.argv[1]
    else:
        mode = 'SquarePlateDiagram'
        
    if mode in ['ReleighDistrib', 
                'SquarePlateDiagram',
                'RepulseDiagram', 
                'СrossDiagram']:
        getattr(sys.modules[__name__], mode)()
        sys.exit(0)
