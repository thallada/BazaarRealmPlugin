#pragma once

RE::TESObjectREFR* PlaceAtMe(RE::BSScript::IVirtualMachine* a_vm, int something, RE::TESObjectREFR* ref, RE::TESForm* form, int count, bool forcePersist, bool initiallyDisabled);
void MoveTo(RE::TESObjectREFR* ref, const RE::ObjectRefHandle& a_targetHandle, RE::TESObjectCELL* a_targetCell, RE::TESWorldSpace* a_selfWorldSpace, const RE::NiPoint3& a_position, const RE::NiPoint3& a_rotation);
