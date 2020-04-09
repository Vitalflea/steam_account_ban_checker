#include <Windows.h>
#include <WinInet.h>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>

struct SMemory {

	char* memory;
	DWORD size;
};

bool downloadInformation(std::string& out, std::string object, bool https = true/*, bool saveToFile = true*/) {

	//debugCmd(/*[+] POST REQUEST: */XorStr<0xA8, 19, 0xE0B99448>("\xF3\x82\xF7\x8B\xFC\xE2\xFD\xFB\x90\xE3\xF7\xE2\xE1\xF0\xE5\xE3\x82\x99" + 0xE0B99448).s + login.useragent + ' ' + login.url + login.object);

	auto hOpen = InternetOpen("account_checker", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hOpen) {

		//winMisc::displayLastError("InternetOpen");

		return false;
	}

	auto hConnect = InternetConnect(hOpen, "www.steamcommunity.com", https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, WININET_API_FLAG_SYNC, 0);
	if (!hConnect) {

		InternetCloseHandle(hOpen);

		//winMisc::displayLastError("InternetConnect");

		return false;
	}

	LPCSTR lplpszTypes[2] = { "*/*", NULL };

	auto requestFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;

	if (https) {

		requestFlags |= INTERNET_FLAG_SECURE;
	}

	auto hRequest = HttpOpenRequest(hConnect, "GET", object.c_str(), NULL, NULL, lplpszTypes, requestFlags, 0);
	if (!hRequest) {

		InternetCloseHandle(hConnect);
		InternetCloseHandle(hOpen);

		//winMisc::displayLastError("HttpOpenRequest");

		return false;
	}

	char lpszHeader[] = "Content-Type: application/x-www-form-urlencoded\r\n";

	if (!HttpSendRequest(hRequest, lpszHeader, -1, NULL, NULL)) {

		InternetCloseHandle(hConnect);
		InternetCloseHandle(hOpen);

		//winMisc::displayLastError("HttpSendRequest");

		return false;
	}

	char responseText[256];
	DWORD responseTextSize = sizeof(responseText);

	if (!HttpQueryInfo(hRequest, HTTP_QUERY_CONTENT_LENGTH, &responseText, &responseTextSize, NULL)) {
		Beep(200, 200);
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hOpen);

		//winMisc::displayLastError("HttpQueryInfo");

		return false;
	}

	SMemory mem = {

		static_cast<char*>(malloc(atoi(responseText))),
		static_cast<DWORD>(atoi(responseText))
	};

	if (!InternetReadFile(hRequest, mem.memory, mem.size, &mem.size)) {

		InternetCloseHandle(hConnect);
		InternetCloseHandle(hOpen);

		//winMisc::displayLastError("InternetReadFile");

		free(mem.memory);

		return false;
	}

	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hOpen);

	out = mem.memory;
	
	free(mem.memory);

	return true;
}

struct AccountInfo {

	std::string m_sAccountName;
	bool m_bPrivate;
	bool m_bOwnsCSGO;
	bool m_bIsBanned;
};

inline bool DoesFileExist(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

INT APIENTRY WinMain(_In_ HINSTANCE /*instance*/, _In_opt_ HINSTANCE /*prevInstance*/,
	_In_ PSTR /*cmdLine*/, _In_ INT /*cmdShow*/) {

	AllocConsole();

	FILE* conOut;
	freopen_s(&conOut, "CONOUT$", "w", stdout);
	freopen_s(&conOut, "CONOUT$", "w", stderr);
	freopen_s(&conOut, "CONIN$", "r", stdin);

	if (DoesFileExist("contas.txt") == false) {

		std::cout << "contas.txt nao encontrado" << '\n';
		std::cout << "formato de conta.txt: username:password" << '\n';

		std::cin.get();

		return 0;
	}

	std::fstream accountFile("contas.txt", std::ios::in | std::ios::app);

	int i = 1;

	std::vector<AccountInfo> accounts;

	std::string account;
	while (std::getline(accountFile, account)) {

		std::cout << "Conta #" << i << " processamento.\n";

		if (auto pos = account.find(":"); pos != std::string::npos) {

			account.erase(pos, account.size());

			AccountInfo accountInfo = { account, false, false, false };

			std::string object = "/id/" + account;

			std::string test = "";
			if (downloadInformation(test, object, true) == false) {

				std::cout << "Conta #" << i << ' ' << account << " nao encontrado." << '\n';
			}
			else {

				std::cout << "Conta #" << i << ' ' << account << " encontrado." << '\n';
			}

			if (test.find("VAC ban on record") != std::string::npos) {

				accountInfo.m_bIsBanned = true;
			}

			test.clear();

			object.append("/inventory/");

			if (downloadInformation(test, object, true) == false
				|| test.find("inventory is currently private") != std::string::npos) {

				std::cout << "Conta #" << i << ' ' << account << " inventario" << " nao encontrado." << '\n';

				accountInfo.m_bPrivate = true;
			}
			else {

				std::cout << "Conta #" << i << ' ' << account << " inventario" << " encontrado." << '\n';
			}

			if (test.find("Counter-Strike: Global Offensive") != std::string::npos) {

				accountInfo.m_bOwnsCSGO = true;
			}

			accounts.emplace_back(accountInfo);
		}

		std::cout << "Conta #" << i++ << " processado.\n\n";
	}

	std::cout << "Todas as contas processadas \n\n";

	accountFile.close();

	std::ofstream accountInfoFile("contas_info.txt", std::ios::binary);

	std::string format = "conta:privado:csgo:banido\n\n";
	accountInfoFile.write(format.c_str(), format.size());

	for (const auto& accountInfo : accounts) {

		std::string con = accountInfo.m_sAccountName;
		con.append(":");
		con.append(std::to_string(accountInfo.m_bPrivate));
		con.append(":");
		con.append(std::to_string(accountInfo.m_bOwnsCSGO));
		con.append(":");
		con.append(std::to_string(accountInfo.m_bIsBanned));
		con.append("\n");

		accountInfoFile.write(con.c_str(), con.size());
	}

	accountInfoFile.close();

	std::cout << "Fim de execucao.\n";

	std::cin.get();

	return 0;
}