import bpy
import os
import json
from bpy.props import (
    BoolProperty,
    CollectionProperty,
    EnumProperty,
    FloatProperty,
    IntProperty,
    PointerProperty,
    StringProperty,
)
from . import material
from ..import utils
from mathutils import Matrix


def export_mesh(exporter, object, materials):
    exporter.try_make_mesh_dir()

    matrix = exporter.correct_matrix()

    if object.type == "MESH":
        b_mesh = object.data
    else:
        b_mesh = object.to_mesh()
    print(object.name, "--------------------------------")
    mat = b_mesh.materials[0]
    mat_data = material.export(exporter, mat, materials)
    ret = {
        "type": "model",
        "names": object.name,
        "param": {
            "fn": exporter.mesh_dir + "/" + object.name + ".glb",
            "smooth": True,
            "material": mat.name,
            "transform": {
                "type": "matrix4x4",
                "param": {
                    "matrix4x4": utils.matrix_to_list(Matrix.Identity(4))
                },
            },
        },
    }
    
    if mat_data is None:
        print("wocaonima   ", object.name)
    
    if mat_data["type"] == "add":
        ret["param"]["emission"] = mat_data["param"]["emission"]
        
    bpy.ops.export_scene.gltf(
        filepath=exporter.mesh_path(object.name),
        #   export_format='GLTF_SEPARATE',
        export_materials="PLACEHOLDER",
        use_selection=True,
    )
    return ret


def export(exporter, object, materials):
    bpy.context.view_layer.objects.active = object
    object.select_set(True)
    ret = export_mesh(exporter, object, materials)
    object.select_set(False)
    return ret
