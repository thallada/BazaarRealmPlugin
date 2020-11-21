#pragma once
RE::NiMatrix3 get_rotation_matrix(RE::NiPoint3 rotation);
RE::NiPoint3 rotate_point(RE::NiPoint3 point, RE::NiMatrix3 rotation_matrix);
