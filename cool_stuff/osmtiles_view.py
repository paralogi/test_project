#!/usr/bin/env python
# -*- coding: utf-8 -*- 

import os
import math
import numpy
import requests
from io import BytesIO
from PIL import Image
from vispy import scene, color, geometry, io
from vispy.visuals import filters, transforms
from vispy.util import transforms as trfunc

class WebMercator(object):
    # to spherical pseudo-mecrator
    def geo2sph(lat):
        y = math.log(math.tan(lat / 2 + math.pi / 4))
        return y

    # from spherical pseudo-mecrator
    def sph2geo(y):
        lat = math.atan(math.exp(y)) * 2 - math.pi / 2
        return lat
    
    # to elliptical mercator
    def geo2merc(lat):
        e = math.sqrt(1 - (6356752.3142 / 6378137) ** 2)
        con = e * math.sin(lat)
        ts = math.tan(0.5 * (math.pi * 0.5 - lat)) / math.pow((1 - con) / (1 + con), 0.5 * e)
        y = -math.log(ts)
        return y
    
    # from elliptical mercator
    def merc2geo(y):
        e = math.sqrt(1 - (6356752.3142 / 6378137) ** 2)
        ts = math.exp(-y)
        lat = math.pi / 2 - 2 * math.atan(ts)
        for i in range(15):
            con = e * math.sin(lat)
            lat0 = lat
            lat = math.pi / 2 - 2 * math.atan(ts * math.pow((1 - con) / (1 + con), 0.5 * e))
            if math.fabs(lat - lat0) > 0.0000000001:
                break
        return lat
    
    # from geo coords to tile numbers
    def deg2num(lon_deg, lat_deg, zoom):
        n = math.pow(2, zoom)
        lat_rad = math.radians(lat_deg)
        xtile = int((lon_deg + 180) / 360 * n)
        ytile = int((1 - math.asinh(math.tan(lat_rad)) / math.pi) / 2 * n)
        return (xtile, ytile)
        
    # to geo coords from tile numbers
    def num2deg(xtile, ytile, zoom):
        n = math.pow(2, zoom)
        lat_rad = math.atan(math.sinh(math.pi * (1 - 2 * ytile / n)))
        lat_deg = math.degrees(lat_rad)
        lon_deg = xtile / n * 360 - 180
        return (lon_deg, lat_deg)

class OsmLoader(object):
    def __init__(self, osm_path=None, osm_url=None, osm_reload=False):
        self.osm_path = osm_path or 'osm'
        self.osm_url = osm_url or 'http://localhost:8080/tile'
        self.osm_reload = osm_reload
        self.data = None       
        self.size = None
        self.rect = None
        self.bounds = None
        
    def loadOSM(self, xtile, ytile, zoom):
        tile = None

        try:
            if os.path.exists(self.osm_path):
                osm_dir = os.path.join(self.osm_path, '{0}/{1}/{2}'.format(zoom, xtile, ytile))
                osm_path = osm_dir + '/data.png'
                
                if not os.path.exists(osm_dir):
                    os.makedirs(osm_dir)
                    
                elif not self.osm_reload and os.path.exists(osm_path):
                    print(osm_path)
                    tile = Image.open(osm_path)
                                        
            if tile is None:
                # osm_url = 'https://tile.openstreetmap.org/{0}/{1}/{2}.png'.format(zoom, xtile, ytile)
                osm_url = os.path.join(self.osm_url, '{0}/{1}/{2}.png'.format(zoom, xtile, ytile))
                osm_head = {'User-Agent':'Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/110.0'}
                print(osm_url)
                
                osm_response = requests.get(osm_url, headers=osm_head)
                osm_response.raise_for_status()
                
                tile = Image.open(BytesIO(osm_response.content))
                if os.path.exists(self.osm_path):
                    tile.save(osm_path)

        except Exception as e: 
            print(e)
            
        return tile
    
    def loadTile(self, xtile, ytile, zoom):
        left, top = WebMercator.num2deg(xtile, ytile, zoom)
        right, bottom = WebMercator.num2deg(xtile + 1, ytile + 1, zoom)
        image = Image.new('RGB', (256, 256))
        image.paste(self.loadOSM(xtile, ytile, zoom), box=(0, 0))
        
        self.set_data(numpy.asarray(image))
        self.size = image.size
        self.rect = geometry.Rect(left, bottom, right - left, top - bottom)
        self.bounds = geomety.Rect(xtile, ytile, 1, 1)
    
    def loadOrigin(self, origin, zoom):
        xtile, ytile = WebMercator.deg2num(origin[0], origin[1], zoom)
        self.loadTile(xtile, ytile, zoom)
    
    def loadPatch(self, xtile0, xtile1, ytile0, ytile1, zoom):
        left, top = WebMercator.num2deg(xtile0, ytile0, zoom)
        right, bottom = WebMercator.num2deg(xtile1 + 1, ytile1 + 1, zoom)
        size = (xtile1 - xtile0 + 1, ytile1 - ytile0 + 1)
        image = Image.new('RGB', (size[0] * 256, size[1] * 256))
        
        for ytile in range(ytile0, ytile1 + 1):
            for xtile in range(xtile0, xtile1 + 1):
                index = (xtile - xtile0, ytile - ytile0)
                image.paste(self.loadOSM(xtile, ytile, zoom), box=(index[0] * 256, index[1] * 256))
                    
        self.data = numpy.asarray(image)
        self.size = image.size
        self.rect = geometry.Rect(left, bottom, right - left, top - bottom)
        self.bounds = geometry.Rect(xtile0, ytile0, xtile1 - xtile0, ytile1 - ytile0)
        
    def loadRect(self, rect, zoom):
        xtile0, ytile1 = WebMercator.deg2num(rect.left, rect.bottom, zoom)
        xtile1, ytile0 = WebMercator.deg2num(rect.right, rect.top, zoom)
        self.loadPatch(xtile0, xtile1, ytile0, ytile1, zoom)

    def loadRegion(self, origin, rect=None, size=None, zoom=None):
        if rect is None:
            self.loadOrigin(origin, 6)
        elif size is None:
            self.loadRect(rect, 6)
        elif zoom is None:
            self.loadRect(rect, self.zoom(rect.size[0], size[0]))
        else:
            self.loadRect(rect, zoom)
            
    def zoom(self, width, size):
        return int(math.log(1.40625 * size / width, 2))
        
class OsmTile(scene.visuals.Image):
    def __init__(self, parent=None):
        super(OsmTile, self).__init__(parent=parent)
        self.set_gl_state(depth_test=False, blend=True)
        self.transform = transforms.STTransform()
        self.unfreeze()
        self.osm_size = None
        self.rect = None
        self.freeze()
        
    def updateTile(self, osm):
        self.set_data(osm.data)
        self.osm_size = osm.size
        self.rect = osm.rect
        return self
        
    def updateView(self):
        self.transform.scale = (self.rect.size[0] / self.osm_size[0], -self.rect.size[1] / self.osm_size[1])
        self.transform.translate = self.rect.left, self.rect.top
        return self

class OsmLayout(scene.Node):
    def __init__(self, parent=None):
        super(OsmLayout, self).__init__(parent=parent)
        self.tiles = []
        self.rect = None
        
    def updateLayout(self, osm, rect, zoom):
        for tile in self.tiles:
            tile.parent = None
        
        self.tiles = []
        self.rect = geometry.Rect(rect)
        progress = 0
        
        xtile0, ytile1 = WebMercator.deg2num(rect.left, rect.bottom, zoom)
        xtile1, ytile0 = WebMercator.deg2num(rect.right, rect.top, zoom)
        
        for ytile in range(ytile0, ytile1 + 1):
            osm.loadPatch(xtile0, xtile1, ytile, ytile, zoom)
            self.tiles.append(OsmTile(parent=self).updateTile(osm))
            
            new_progress = int(100 * (ytile - ytile0) / (ytile1 - ytile0))
            if progress != new_progress:
                progress = new_progress
                print(progress, '%', sep='')
                
        return self
    
    def updateView(self):
        for tile in self.tiles:
            tile.updateView()
        return self

class OsmView(scene.SceneCanvas):
    def __init__(self, parent=None):
        super(OsmView, self).__init__(keys='interactive')
        self.unfreeze()
        self.view = self.central_widget.add_view()
        self.view.camera = scene.PanZoomCamera(aspect=1)
        self.view.camera.transform.changed.connect(self.on_view_changed)
        self.osm_loader = OsmLoader()
        self.osm_layout = OsmLayout(parent=self.view.scene)
        self.grid_lines = scene.visuals.GridLines(color='black', parent=self.view.scene)
        self.grid_lines.set_gl_state(depth_test=False)
        self.origin_mark = scene.Markers(parent=self.view.scene)
        self.origin_mark.visible = False
        self.axx = scene.visuals.Axis(axis_color='black', tick_color='black', text_color='black', tick_direction=(0, 1), parent=self.view.scene)
        self.axy = scene.visuals.Axis(axis_color='black', tick_color='black', text_color='black', tick_direction=(1, 0), parent=self.view.scene)
        self.freeze()
    
    def updateOsm(self, rect, zoom=None):
        rect = rect
        osm_zoom = zoom or self.osm_loader.zoom(rect.size[0], self.view.size[0])
        self.osm_layout.updateLayout(self.osm_loader, rect, osm_zoom).updateView()
        self.origin_mark.set_data(pos=numpy.array([[*rect.center]]), symbol='x', face_color='white')
        self.view.camera.rect = rect
        return self

    def on_view_changed(self, event):
        viewport = self.view.camera.rect
        with self.axx.events.blocker():
            self.axx.pos = numpy.array([[viewport.left, viewport.bottom], [viewport.right, viewport.bottom]])
            self.axx.domain = (viewport.left, viewport.right)
        with self.axy.events.blocker():
            self.axy.pos = numpy.array([[viewport.left, viewport.bottom], [viewport.left, viewport.top]])
            self.axy.domain = (viewport.bottom, viewport.top)
            
    def on_resize(self, event):
        super(OsmView, self).on_resize(event)
        center = self.view.camera.center
        zoom = self.size[0] / self.size[1]
        self.view.camera.rect.width = zoom * self.view.camera.rect.height
        self.view.camera.rect.center = center
        self.view.camera.view_changed()

if __name__ == '__main__':
    osm_rect = geometry.Rect(37.61556 - 0.5, 55.75222 - 0.5, 1, 1)

    osm_view = OsmView()
    osm_view.size = 800, 800
    osm_view.updateOsm(osm_rect, 10)
    osm_view.show(run=True)
