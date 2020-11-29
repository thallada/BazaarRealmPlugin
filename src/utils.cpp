RE::NiMatrix3 get_rotation_matrix(RE::NiPoint3 rotation) {
	RE::NiMatrix3 rotation_matrix = RE::NiMatrix3();
	rotation_matrix.entry[0][0] = cos(rotation.z) * cos(rotation.y);
	rotation_matrix.entry[0][1] = (cos(rotation.z) * sin(rotation.y) * sin(rotation.x)) - (sin(rotation.z) * cos(rotation.x));
	rotation_matrix.entry[0][2] = (cos(rotation.z) * sin(rotation.y) * cos(rotation.x)) + (sin(rotation.z) * sin(rotation.x));
	rotation_matrix.entry[1][0] = sin(rotation.z) * cos(rotation.y);
	rotation_matrix.entry[1][1] = (sin(rotation.z) * sin(rotation.y) * sin(rotation.x)) + (cos(rotation.z) * cos(rotation.x));
	rotation_matrix.entry[1][2] = (sin(rotation.z) * sin(rotation.y) * cos(rotation.x)) - (cos(rotation.z) * sin(rotation.x));
	rotation_matrix.entry[2][0] = sin(rotation.y) * -1;
	rotation_matrix.entry[2][1] = cos(rotation.y) * sin(rotation.x);
	rotation_matrix.entry[2][2] = cos(rotation.y) * cos(rotation.x);
	return rotation_matrix;
}

RE::NiPoint3 rotate_point(RE::NiPoint3 point, RE::NiMatrix3 rotation_matrix) {
	return RE::NiPoint3(
		(point.x * rotation_matrix.entry[0][0]) + (point.y * rotation_matrix.entry[1][0]) + (point.z * rotation_matrix.entry[2][0]),
		(point.x * rotation_matrix.entry[0][1]) + (point.y * rotation_matrix.entry[1][1]) + (point.z * rotation_matrix.entry[2][1]),
		(point.x * rotation_matrix.entry[0][2]) + (point.y * rotation_matrix.entry[1][2]) + (point.z * rotation_matrix.entry[2][2])
	);
}

std::pair<uint32_t, const char*> get_local_form_id_and_mod_name(RE::TESForm* form) {
	RE::FormID form_id = form->GetFormID();
	RE::TESFile* file = form->GetFile(0);
	const char* mod_name = file->fileName;
	bool is_light = file->recordFlags.all(RE::TESFile::RecordFlag::kSmallFile);
	return std::pair(is_light ? form_id & 0xfff : form_id & 0xFFFFFF, mod_name);
}
