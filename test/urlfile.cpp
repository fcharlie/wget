#include <string>
#include <iostream>

bool RemoteFileSplitName(const std::wstring &url,std::wstring &file) {
	auto np = url.rfind('/');
	if (np == url.npos)
		return false;
	auto xnp = url.find('?', np);
	if (xnp == url.npos) {
		file.assign(url.substr(np + 1));
	}
	else {
		file.assign(url.substr(np + 1, xnp - np-1));
	}
	return true;
}

const wchar_t * urls[]={
	L"https://github.com/fcharlie/wget/release/master.zip",
	L"https://download.example.io/example/bin.zip?hash=zzzz"
};

int wmain(){
	std::wstring name;
	for(auto &u:urls){
		if(RemoteFileSplitName(u,name)){
			std::wcout<<L"URL: "<<u<<L" Name: "<<name<<std::endl;
		}
	}
	return 1;
}