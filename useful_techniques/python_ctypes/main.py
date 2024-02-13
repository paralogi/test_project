import os
import ctypes

class Resource(ctypes.Structure):
    _fields_ = [
        ('model_hash', ctypes.ARRAY(ctypes.c_char, 32)),
        ('model_file', ctypes.ARRAY(ctypes.c_char, 256)),
        ('model_range', ctypes.c_double)]

    def __repr__(self):
        return f'model_hash={self.model_hash}\nmodel_file={self.model_file}\nmodel_range={self.model_range:.2f}'

dll = ctypes.CDLL(os.path.join(os.path.dirname(__file__), './test.so'))
dll.globalResource.restype = ctypes.POINTER(Resource)
print(dll.globalResource()[0])
