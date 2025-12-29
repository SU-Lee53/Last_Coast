#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include "AssimpConverter.h"
#include <commdlg.h>
#include <shlobj.h>

HWND	g_hOpenButton;	// Open Button
HWND	g_hConvertButton;	// Convert Button
HWND	g_hMainEdit;	// Main Edit Control (Read Only)
HWND	g_hScaleEdit;	// Scale Factor Edit
HWND	g_hModelRadio;	// Model Radio
HWND	g_hAnimRadio;	// Animation Radio

TCHAR g_str[1024] = L"";
TCHAR g_lpstrFile[1024] = L"";
TCHAR g_lpstrFilter[256] = L"fbx File(*.fbx)\0*.fbx\0";
TCHAR g_lpstrFileTitle[256] = L"";

bool g_bModelConvertOn = false;
bool g_bAnimationConvertOn = false;

AssimpConverter g_Converter;


INT_PTR CALLBACK	DlgProc(HWND, UINT, WPARAM, LPARAM);
void DisplayText(const char* fmt, ...);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// 대화상자 생성
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	return 0;
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	OPENFILENAME OFN;
	BROWSEINFOW bi = {};
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

	switch (uMsg) {
	case WM_INITDIALOG:
		g_hOpenButton = GetDlgItem(hDlg, IDC_BUTTON1);
		g_hConvertButton = GetDlgItem(hDlg, IDC_BUTTON4);
		g_hMainEdit = GetDlgItem(hDlg, IDC_EDIT1);
		g_hScaleEdit = GetDlgItem(hDlg, IDC_EDIT2);
		g_hModelRadio = GetDlgItem(hDlg, IDC_RADIO1);
		g_hAnimRadio = GetDlgItem(hDlg, IDC_RADIO2);

		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON1:
		{
			::ZeroMemory(&OFN, sizeof(OPENFILENAME));
			OFN.lStructSize = sizeof(OPENFILENAME);
			OFN.hwndOwner = hDlg;
			OFN.lpstrFilter = g_lpstrFilter;
			OFN.lpstrFile = g_lpstrFile;
			OFN.nMaxFile = 256;
			OFN.lpstrFileTitle = g_lpstrFileTitle;
			OFN.nMaxFileTitle = 256;
			OFN.lpstrInitialDir = L".";

			if (GetOpenFileName(&OFN) != 0) {
				wsprintf(g_str, L"Open \"%s\" ?", OFN.lpstrFile);
				MessageBox(hDlg, g_str, L"열기", MB_OK);
			}

			wchar_t buffer[64] = {};
			GetWindowTextW(g_hScaleEdit, buffer, _countof(buffer));
			float fValue = static_cast<float>(_wtof(buffer));
			fValue = fValue <= 0.f ? 1.f : fValue;

			char cstrFilepath[1024];
			WideCharToMultiByte(CP_ACP, 0, g_lpstrFile, -1, cstrFilepath, sizeof(cstrFilepath), NULL, NULL);

			g_Converter.LoadFromFiles(cstrFilepath, fValue);
			DisplayText("File opened (%s)\r\n", cstrFilepath);

			return TRUE;
		}
		case IDC_BUTTON4:
		{
			if (!g_Converter.IsOpened()) {
				DisplayText("Nothing is opened yet!!\n");
				return TRUE;
			}

			char cstrFilepath[1024];
			LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
			if (pidl)
			{
				TCHAR path[MAX_PATH];
				if (SHGetPathFromIDListW(pidl, path))
				{
					WideCharToMultiByte(CP_ACP, 0, path, -1, cstrFilepath, sizeof(cstrFilepath), NULL, NULL);
				}
				CoTaskMemFree(pidl);
			}

			char cstrFilename[1024];
			WideCharToMultiByte(CP_ACP, 0, g_lpstrFileTitle, -1, cstrFilename, sizeof(cstrFilename), NULL, NULL);
			std::filesystem::path p{ cstrFilename };
			std::string strExportName = p.stem().string();

			g_Converter.Serialize(cstrFilepath, strExportName);
			DisplayText("Successfully serialized at %s\r\n", cstrFilepath);
			return TRUE;
		}
		case IDC_RADIO1:
		{
			g_bModelConvertOn = (IsDlgButtonChecked(hDlg, IDC_RADIO1) == BST_CHECKED);
			return TRUE;
		}
		case IDC_RADIO2:
		{
			g_bAnimationConvertOn = (IsDlgButtonChecked(hDlg, IDC_RADIO2) == BST_CHECKED);
			return TRUE;
		}
		case IDOK:
		{
			return TRUE;
		}
		case IDCANCEL:
		{
			EndDialog(hDlg, IDCANCEL); // 대화상자 닫기
			return TRUE;
		}
		}
		return FALSE;
	}
	return FALSE;
}

void DisplayText(const char* fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);
	char cbuf[512 * 2];
	vsprintf_s(cbuf, fmt, arg);
	va_end(arg);

	int nLength = GetWindowTextLength(g_hMainEdit);
	SendMessage(g_hMainEdit, EM_SETSEL, nLength, nLength);
	SendMessageA(g_hMainEdit, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
}
