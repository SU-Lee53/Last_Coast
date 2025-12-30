#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include "AssimpConverter.h"
#include <commdlg.h>
#include <shlobj.h>
#include <shobjidl.h>   // IFileDialog

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
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		return -1;
	}

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
		CheckDlgButton(hDlg, IDC_RADIO1, BST_CHECKED);
		SetDlgItemTextW(hDlg, IDC_EDIT2, L"1.0");

		g_bModelConvertOn = (IsDlgButtonChecked(hDlg, IDC_RADIO1) == BST_CHECKED);
		g_bAnimationConvertOn = (IsDlgButtonChecked(hDlg, IDC_RADIO2) == BST_CHECKED);

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
				MessageBox(hDlg, g_str, L"Open", MB_OK);
			}
			else {
				return TRUE;
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

			char cstrFilepath[1024] = {};

			IFileDialog* pDialog = nullptr;
			HRESULT hr = CoCreateInstance(
				CLSID_FileOpenDialog,
				nullptr,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&pDialog)
			);

			if (FAILED(hr))
				return TRUE;

			// Set Options
			DWORD options;
			pDialog->GetOptions(&options);
			pDialog->SetOptions(
				options |
				FOS_PICKFOLDERS |        // Select folder
				FOS_FORCEFILESYSTEM     // Only filesystem paths
			);

			hr = pDialog->Show(hDlg);
			if (SUCCEEDED(hr))
			{
				IShellItem* pItem = nullptr;
				if (SUCCEEDED(pDialog->GetResult(&pItem)))
				{
					PWSTR pszPath = nullptr;
					if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)))
					{
						WideCharToMultiByte(
							CP_ACP,
							0,
							pszPath,
							-1,
							cstrFilepath,
							sizeof(cstrFilepath),
							nullptr,
							nullptr
						);
						CoTaskMemFree(pszPath);
					}
					pItem->Release();
				}
			}
			else
			{
				pDialog->Release();
				return TRUE; // Cancel
			}

			pDialog->Release();

			char cstrFilename[1024];
			WideCharToMultiByte(CP_ACP, 0, g_lpstrFileTitle, -1, cstrFilename, sizeof(cstrFilename), NULL, NULL);
			std::filesystem::path p{ cstrFilename };
			std::string strExportName = p.stem().string();

			if (g_bModelConvertOn) {
				g_Converter.SerializeModel(cstrFilepath, strExportName);
			}
			else {
				g_Converter.SerializeAnimation(cstrFilepath, strExportName);
			}

			wsprintf(g_str, L"Conversion Complete");
			MessageBox(hDlg, g_str, L"Convert", MB_OK);

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
