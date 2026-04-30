#include "pch.h"
#include "Utility.h"

void ShowErrorMessage(std::string_view file, int line, std::string_view message)
{
	std::string strFileMsg{ file };
	strFileMsg += '\n';

	std::string strLineMsg = "Line : " + std::to_string(line);
	strLineMsg += '\n';

	std::string strDebugMsg{ message };
	strDebugMsg += '\n';

	::OutputDebugStringA("*************** ERROR!! ***************\n");
	::OutputDebugStringA(strFileMsg.data());
	::OutputDebugStringA(strLineMsg.data());
	::OutputDebugStringA(strDebugMsg.data());
	::OutputDebugStringA("***************************************\n");
}


void ImGuiHelper::PrintVector2(const char* label, const Vector2& v)
{
	ImGui::Text("%s : (%.3f, %.3f)", label, v.x, v.y);
}

void ImGuiHelper::PrintVector3(const char* label, const Vector3& v)
{
	ImGui::Text("%s : (%.3f, %.3f, %.3f)", label, v.x, v.y, v.z);
}

void ImGuiHelper::PrintVector4(const char* label, const Vector4& v)
{
	ImGui::Text("%s : (%.3f, %.3f, %.3f, %.3f)", label, v.x, v.y, v.z, v.w);
}

void ImGuiHelper::PrintQuaternion(const char* label, const Quaternion& q)
{
	ImGui::Text("%s : (%.3f, %.3f, %.3f, %.3f)", label, q.x, q.y, q.z, q.w);
}

void ImGuiHelper::PrintMatrix(const char* label, const Matrix& m)
{
	ImGui::Text("%s", label);

	ImGui::Text("[ % .3f  % .3f  % .3f  % .3f ]", m._11, m._12, m._13, m._14);
	ImGui::Text("[ % .3f  % .3f  % .3f  % .3f ]", m._21, m._22, m._23, m._24);
	ImGui::Text("[ % .3f  % .3f  % .3f  % .3f ]", m._31, m._32, m._33, m._34);
	ImGui::Text("[ % .3f  % .3f  % .3f  % .3f ]", m._41, m._42, m._43, m._44);
}

void ImGuiHelper::PrintTransformMatrix(const char* label, const Matrix& m)
{
	ImGui::Text("%s", label);

	ImGui::Text("Right   : (%.3f, %.3f, %.3f)", m._11, m._12, m._13);
	ImGui::Text("Up      : (%.3f, %.3f, %.3f)", m._21, m._22, m._23);
	ImGui::Text("Forward : (%.3f, %.3f, %.3f)", m._31, m._32, m._33);
	ImGui::Text("Pos     : (%.3f, %.3f, %.3f)", m._41, m._42, m._43);
}
