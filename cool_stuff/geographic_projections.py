# -*- coding: utf-8 -*-

import sys
import math
import numpy
import copy
from youtube_dl.extractor import inc

numpy.set_printoptions(precision=8, suppress=True)

class Position(object):
    # set position coordinates
    def __init__(self, x, y, z):
        self.xyz = numpy.array([float(x), float(y), float(z)])
        self.h = None
        self.r = None
        self.a = None
        
    @property
    def x(self):
        return self.xyz[0]
    
    @x.setter
    def x(self, value):
        self.xyz[0] = value
    
    @property
    def y(self):
        return self.xyz[1]
    
    @y.setter
    def y(self, value):
        self.xyz[1] = value
    
    @property
    def z(self):
        return self.xyz[2]
    
    @z.setter
    def z(self, value):
        self.xyz[2] = value
        
    # calculate hypot
    def hypot(self):
        self.h = math.hypot(self.x, self.y)
        return self
    
    # calculate range
    def range(self):
        self.r = math.sqrt(self.x**2 + self.y**2 + self.z**2)
        return self
    
    # calculate azimuth
    def azimuth(self):
        self.a = numpy.arctan2(self.x, self.y)
        return self
    
    # calculate tangent angle
    def angle(self):
        if self.h != 0:
            self.a = self.z / self.h
        else:
            self.a = -sys.float_info.max
        return self

class Mesh(object):
    # set points on plain
    def __init__(self):
        self.xyz = None
        self.h = None
        self.r = None
        self.n = None
        self.a = None
        self.s = None
        
    @property
    def x(self):
        return self.xyz[0]
    
    @x.setter
    def x(self, value):
        self.xyz[0] = value
    
    @property
    def y(self):
        return self.xyz[1]
    
    @y.setter
    def y(self, value):
        self.xyz[1] = value
    
    @property
    def z(self):
        return self.xyz[2]
    
    @z.setter
    def z(self, value):
        self.xyz[2] = value
        
    #
    def copy(self):
        return copy.deepcopy(self)

    # 
    def setXYZ(self, xyz):
        self.xyz = xyz
        return self
    
    # 
    def setCoord(self, x, y, z):
        self.xyz = numpy.array([x, y, z])
        return self

    # calculate hypot
    def hypots(self):
        self.h = numpy.hypot(self.x, self.y)
        return self
    
    # calculate range
    def ranges(self):
        self.r = numpy.sqrt(numpy.sum(self.xyz**2, axis=0))
        return self
    
    # calculate azimuth
    def azimuths(self):
        self.a = numpy.arctan2(self.x, self.y)
        return self
    
    # calculate tangent angles
    def angles(self):
        mask = self.h != 0
        self.a = numpy.empty_like(self.z)
        self.a[mask] = self.z[mask] / self.h[mask]
        self.a[~mask] = -sys.float_info.max
        return self
    
    # calculate sight mask
    def sights(self):
        self.a = numpy.maximum.accumulate(self.a)
        self.s = numpy.concatenate(([False], self.a[1:] != self.a[:-1]))
        
        sights = numpy.flatnonzero(self.s)
        self.xyz = self.xyz[:, sights[0]:sights[-1]+1]
        self.h = self.h[sights[0]:sights[-1]+1]
        self.a = self.a[sights[0]:sights[-1]+1]
        self.s = self.s[sights[0]:sights[-1]+1]
        
        sights = numpy.flatnonzero(self.s)
        nsights = numpy.flatnonzero(~self.s)
        intervals = numpy.concatenate((sights[1:] - sights[:-1] - 1, [0]))
        breaks = numpy.flatnonzero(intervals != 0)
        sights0 = sights[breaks]
        sights1 = sights[breaks + 1]
        sights2 = sights1 - 1
        
        h0 = self.h[sights0]
        h1 = self.h[sights1]
        h2 = self.h[sights2]
        z0 = self.z[sights0]
        z1 = self.z[sights1]
        z2 = self.z[sights2]
        a = self.a[sights0]
        b = (z2 - z1) / (h2 - h1)
        c = z0 - a * h0
        d = z1 - b * h1
        h = (d - c) / (a - b)
        z = a * h + c
        
        repeats = intervals[breaks]
        h0 = numpy.repeat(h0, repeats=repeats)
        h1 = numpy.repeat(h, repeats=repeats)
        z0 = numpy.repeat(z0, repeats=repeats)
        z1 = numpy.repeat(z, repeats=repeats)
        
        self.h[sights2] = h
        self.x[sights2] *= h / h2
        self.y[sights2] *= h / h2
        self.z[nsights] = z0 + (z1 - z0) * (self.h[nsights] - h0) / (h1 - h0)
        return self
    
    # calculate plane derivative
    def derivations(self):
        dz = self.z[:-1] - self.z[1:]
        i = numpy.argwhere(dz != 0)
        j = numpy.argwhere(dz == 0) + 1
        k = numpy.argwhere(dz < 0) + 1
        u = numpy.empty((3, self.z.size), dtype=float)
        u[0] = self.x / self.h
        u[0, j] = 0
        u[1] = self.y / self.h
        u[1, j] = 0
        u[2, i + 1] = (self.h[1:] - self.h[:-1])[i] / dz[i]
        u[2, j] = 1
        u[:, k] = -u[:, k]
        s = numpy.sqrt(numpy.sum(u**2, axis=0))
        self.n = (u / s).transpose((1, 0))
        return self
    
    # calculate surface normal_radius
    def normals(self):
        i, j = numpy.unravel_index(numpy.arange(self.z.size), shape=self.z.shape)
        v = self.xyz.transpose((1, 2, 0))
        u = numpy.empty((4, v.shape[0] * v.shape[1], v.shape[-1]), dtype=float)
        u[0] = v[numpy.fmax(i - 1, 0), j]
        u[1] = v[i, numpy.fmin(j + 1, self.z.shape[1] - 1)]
        u[2] = v[numpy.fmin(i + 1, self.z.shape[0] - 1), j]
        u[3] = v[i, numpy.fmax(j - 1, 0)]
        u = u - v[i, j]
        u = numpy.cross(u[0], u[-1]) + numpy.cross(u[1], u[0]) + numpy.cross(u[2], u[1]) + numpy.cross(u[3], u[2])
        s = numpy.sqrt(numpy.sum(u**2, axis=-1))
        self.n = (u / s[:, None]).reshape(v.shape)
        return self
    
class Projection(object):
    # set projection constants
    def __init__(self, origin, scale):
        self.origin = origin
        self.sin_lon0 = math.sin(math.radians(origin.x))
        self.cos_lon0 = math.cos(math.radians(origin.x))
        self.sin_lat0 = math.sin(math.radians(origin.y))
        self.cos_lat0 = math.cos(math.radians(origin.y))
        self.flattening = 1. / 298.257223563
        self.eccentricity1 = self.flattening * (2. - self.flattening)
        self.eccentricity2 = self.eccentricity1 / (1. - self.eccentricity1)
        self.latitude_factor = math.sqrt(1. + self.eccentricity2 * self.cos_lat0**2)
        self.scale_factor = scale
        self.major_radius = 6378137. / self.scale_factor
        self.minor_radius = self.major_radius * (1. - self.flattening)
        self.polar_radius = self.major_radius / (1. - self.flattening)
        self.normal_radius = self.polar_radius / self.latitude_factor
        # self.meridian_radius = self.polar_radius * (1 - self.eccentricity1) / self.latitude_factor**3

    # helper rotation function
    def rotate(x, y, cos, sin):
        return (x * cos + y * sin, -x * sin + y * cos)
    
    # projection epsg:54032 to epsg:4326
    def sph2geod(self, azimuth, distance):
        alpha = math.atan(math.tan(azimuth) / self.latitude_factor)
        
        a = self.major_radius
        b = self.major_radius * math.sqrt(1 - self.eccentricity1 * math.cos(alpha)**2)
        f = (a - b) / a
        e1 = f * (2 - f)
        e2 = e1 / (1 - e1)
        k2 = e2 * math.cos(alpha)**2
        eps = k2 / (math.sqrt(1 + k2) + 1)**2
        A1 = (1 + 1/4 * eps**2 + 1/64 * eps**4 + 1/256 * eps**6 + 25/16384 * eps**8 + 49/65536 * eps**10) / (1 - eps)
        C1 = numpy.array([-1/2 * eps + 3/16 * eps**3 - 1/32 * eps**5 + 19/2048 * eps**7 - 3/4096 * eps**9,
              1/16 * eps**2 + 1/32 * eps**4 - 9/2048 * eps**6 + 7/4096 * eps**8 + 1/65536 * eps**10,
              -1/48 * eps**3 + 3/256 * eps**5 - 3/2048 * eps**7 + 17/24576 * eps**9,
              -5/512 * eps**4 + 3/512 * eps**6 - 11/16384 * eps**8 + 3/8192 * eps**10,
              7/1280 * eps**5 + 7/2048 * eps**7 - 3/8192 * eps**9,
              -7/2048 * eps**6 + 9/4096 * eps**8 - 117/524288 * eps**10,
              -33/14336 * eps**7 + 99/65536 * eps**9,
              -429/262144 * eps**8 + 143/131072 * eps**10,
              -715/589824 * eps**9,
              -2431/2621440 * eps**10])
        
        delta = distance / self.major_radius
        delta = A1 * (delta + numpy.sum(C1 * numpy.sin(2 * delta * numpy.arange(len(C1)))))
        
        psi = math.pi - alpha
        phi = math.pi / 2 - delta
        x = math.cos(phi) * math.cos(psi)
        y = math.cos(phi) * math.sin(psi)
        z = math.sin(phi)
        
        lon1 = self.origin.x
        lat1 = math.atan(math.tan(self.origin.y) * (1 - self.flattening))
        z, x = Projection.rotate(z, x, math.sin(lat1), -math.cos(lat1))
        x, y = Projection.rotate(x, y, math.cos(lon1), -math.sin(lon1))
        
        lon2 = math.atan2(y, x)
        lat2 = math.atan((z / math.hypot(x, y)) / (1 - self.flattening))
        return (lon2, lat2)
        
    # projection epsg:4326 to epsg:4978
    def geod2ecef(self, object):
        longitude = numpy.radians(self.origin.x + (object.x - self.origin.x) * self.scale_factor)
        latitude = numpy.radians(self.origin.y + (object.y - self.origin.y) * self.scale_factor)
        altitude = object.z
        coslat = numpy.cos(latitude) 
        sinlat = numpy.sin(latitude)
        normal = self.polar_radius / numpy.sqrt(1. + self.eccentricity2 * coslat**2)
        hypot = (normal + altitude) * coslat
        object.x = hypot * numpy.cos(longitude)
        object.y = hypot * numpy.sin(longitude)
        object.z = (normal + altitude - self.eccentricity1 * normal) * sinlat
        
    # projection epsg:4978 to epsg:4326
    def ecef2geod(self, object):
        hypot = numpy.hypot(object.x, object.y)
        tanlat = object.z / hypot * (1. + self.eccentricity2 * self.minor_radius / numpy.hypot(hypot, object.z))
        for i in range(2):
            tanlat = tanlat * (1. - self.flattening)
            latitude = numpy.arctan(tanlat)
            coslat = numpy.cos(latitude) 
            sinlat = numpy.sin(latitude)
            tanlat = ((object.z + self.eccentricity2 * self.minor_radius * sinlat**3) / 
                      (hypot - self.eccentricity1 * self.major_radius * coslat**3))
        longitude = numpy.arctan2(object.y, object.x)
        latitude = numpy.arctan(tanlat)
        coslat = numpy.where(latitude == math.pi / 2, 1, numpy.cos(latitude))
        sinlat = numpy.where(latitude == 0, 1, numpy.sin(latitude))
        normal = self.polar_radius / numpy.sqrt(1. + self.eccentricity2 * coslat**2)
        altitude = numpy.where(numpy.fabs(tanlat) <= 1., hypot / coslat - normal, 
                               object.z / sinlat - normal * (1. - self.eccentricity1))
        object.x = self.origin.x + (numpy.degrees(longitude) - self.origin.x) / self.scale_factor
        object.y = self.origin.y + (numpy.degrees(latitude) - self.origin.y) / self.scale_factor
        object.z = altitude

    # projection epsg:4978 to epsg:5819
    def ecef2topo(self, object):
        object.z = object.z + self.eccentricity1 * self.normal_radius * self.sin_lat0
        object.x, object.y = Projection.rotate(object.x, object.y, self.cos_lon0, self.sin_lon0)
        object.z, object.x = Projection.rotate(object.z, object.x, self.sin_lat0, self.cos_lat0)
        object.x, object.y = object.y.copy(), -object.x.copy()
        object.z = object.z - (self.normal_radius + self.origin.z)
        
    # projection epsg:5819 to epsg:4978
    def topo2ecef(self, object):
        object.z = object.z + (self.normal_radius + self.origin.z)
        object.x, object.y = -object.y.copy(), object.x.copy()
        object.z, object.x = Projection.rotate(object.z, object.x, self.sin_lat0, -self.cos_lat0)
        object.x, object.y = Projection.rotate(object.x, object.y, self.cos_lon0, -self.sin_lon0)
        object.z = object.z - self.eccentricity1 * self.normal_radius * self.sin_lat0
    
     # altitude in global coordinates
    def globalAltitude(self, altitude):
        return self.normal_radius + altitude
    
    # horizon range in global coordinates
    def globalHorizon(self, altitude):
        return math.sqrt(2 * altitude / self.normal_radius)
    
    # range in global coordinates
    def globalRange(self, range, altitude=0):
        return math.degrees(range / (self.normal_radius + altitude))
    
    # local projection with copy
    def localCopy(self, obj): 
        dup = copy.deepcopy(obj)
        self.geod2ecef(dup)
        self.ecef2topo(dup)
        return dup
    
    # local projection in place
    def localRef(self, obj): 
        dup = copy.copy(obj)
        self.geod2ecef(dup)
        self.ecef2topo(dup)
        return dup
    
    # global projection with copy
    def globalCopy(self, obj): 
        dup = copy.deepcopy(obj)
        self.topo2ecef(dup)
        self.ecef2geod(dup)
        return dup
    
    # global projection in place
    def globalRef(self, obj): 
        dup = copy.copy(obj)
        self.topo2ecef(dup)
        self.ecef2geod(dup)
        return dup
    
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
    
numpy.set_printoptions(suppress=True, precision=3)
proj_real = Projection(Position(0, 0, 0), 1)
proj_curv = Projection(Position(0, 0, 0), 0.75)

distance = 100000
altitude = 0000
azimuth = math.radians(45)
cosax = math.sin(azimuth)
cosay = math.cos(azimuth)

point = Position(cosax * distance, cosay * distance, 0)
proj_real.globalRef(point).z = altitude
proj_real.localRef(point)
print(math.degrees(point.azimuth().a), math.fabs(azimuth - point.a) * distance)

point = Position(*proj_real.sph2geod(azimuth, distance), altitude)
proj_real.localRef(point)
print(math.degrees(point.azimuth().a), math.fabs(azimuth - point.a) * distance)

# ---
point1 = Position(cosax * distance, cosay * distance, 0)
proj_real.globalRef(point1).z = 0
proj_real.localRef(point1)
print('point1', point1.xyz, math.degrees(math.atan(point1.hypot().angle().a)))

exit(0)
