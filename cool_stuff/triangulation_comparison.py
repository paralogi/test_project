#!/usr/bin/env python
# -*- coding: utf-8 -*-

import math
import numpy
from vispy import app, gloo
from vispy.color import get_colormap
from vispy.visuals import LineVisual
from vispy.visuals.collections import PathCollection

def line_visual(points):
    vertices = numpy.vstack((points[:, :2], points[0, :2]))
    print('vertices', *list(enumerate(vertices)), sep='\n')
    
    paths = LineVisual(pos=vertices, color='coolwarm', method='gl', antialias=True)
    gloo.wrappers.set_line_width(width=3)
    return paths

def path_visual(points):
    edges = numpy.repeat(numpy.arange(len(points) + 1), 2)[1:-1]
    edges[-2:] = len(points) - 1, 0
    edges = edges.reshape(len(edges) // 2, 2)
    print('edges', *list(enumerate(edges)), sep='\n')
        
    colors = get_colormap('coolwarm').map(numpy.linspace(0., 1., len(edges)))
    paths = PathCollection(mode='agg', color='shared')
    paths['linewidth'] = 3
    
    for index in range(len(edges)):
        paths.append(points[edges[index]], closed=False, color=colors[index])
        
    return paths

def triangulate_self(points):
    vertices = numpy.vstack((numpy.mean(points, axis=0), points))
    print('vertices', *list(enumerate(vertices)), sep='\n')
    
    faces = numpy.zeros((len(points), 3), dtype=numpy.uint32)
    faces[:, 1] = numpy.arange(len(points)) + 1
    faces[:, 2] = faces[:, 1] + 1
    faces[-1, 2] = 1
    print('faces', *list(enumerate(faces)), sep='\n')
    
    colors = get_colormap('coolwarm').map(numpy.linspace(0., 1., len(faces)))
    paths = PathCollection(mode='agg', color='shared')
    paths['linewidth'] = 3

    for index in range(len(faces)):
        triangle = vertices[faces[index]]
        center = numpy.mean(triangle, axis=0)
        distance = math.pow(0.9, numpy.linalg.norm(center)) * 1.1
        paths.append(triangle + center * (distance - 1), closed=True, color=colors[index])
        
    return paths

def triangulate_vispy(points):
    edges = numpy.repeat(numpy.arange(len(points) + 1), 2)[1:-1]
    edges[-2:] = len(points) - 1, 0
    edges = edges.reshape(len(edges) // 2, 2)
    print('edges', *list(enumerate(edges)), sep='\n')
        
    from vispy.geometry import Triangulation
    triangulation = Triangulation(points, edges)
    triangulation.triangulate()

    vertices = numpy.zeros((len(triangulation.pts), 3))
    vertices[:, :2] = triangulation.pts
    print('vertices', *list(enumerate(vertices)), sep='\n')
    
    faces = triangulation.tris
    print('faces', *list(enumerate(faces)), sep='\n')
    
    colors = get_colormap('coolwarm').map(numpy.linspace(0., 1., len(faces)))
    paths = PathCollection(mode='agg', color='shared')
    paths['linewidth'] = 3
    
    for index in range(len(faces)):
        triangle = vertices[faces[index]]
        center = numpy.mean(triangle, axis=0)
        distance = math.pow(0.9, numpy.linalg.norm(center)) * 1.1
        paths.append(triangle + center * (distance - 1), closed=True, color=colors[index])
        
    return paths

def triangulate_shapely(points):
    from shapely.ops import triangulate
    from shapely.geometry import MultiPoint, Polygon
    triangles = triangulate(MultiPoint(points))
    
    colors = get_colormap('coolwarm').map(numpy.linspace(0., 1., len(triangles)))
    paths = PathCollection(mode='agg', color='shared')
    paths['linewidth'] = 3
    
    for index, triangle in enumerate(triangles):
        x, y = triangle.exterior.coords.xy
        vertices = numpy.zeros((3, 3))
        vertices[:, 0] = x[:3]
        vertices[:, 1] = y[:3]
        center = numpy.mean(vertices, axis=0)
        distance = math.pow(0.9, numpy.linalg.norm(center)) * 1.1
        paths.append(vertices + center * (distance - 1), closed=True, color=colors[index])
        
    return paths

def star(inner=0.4, outer=0.8, n=12):
    R = numpy.array([inner, outer] * n)
    T = numpy.linspace(0, 2 * numpy.pi, 2 * n, endpoint=False)
    P = numpy.zeros((2 * n, 3))
    P[:, 0] = R * numpy.cos(T)
    P[:, 1] = R * numpy.sin(T)
    return P

canvas = app.Canvas(size=(600, 600), show=False, keys='interactive')
gloo.set_viewport(0, 0, canvas.size[0], canvas.size[1])
gloo.set_state('translucent', depth_test=False)
paths = line_visual(star())

@canvas.connect
def on_draw(e):
    gloo.clear('white')
    paths.draw()
    
@canvas.connect
def on_resize(event):
    width, height = event.size
    gloo.set_viewport(0, 0, width, height)
    
canvas.show()
app.run()