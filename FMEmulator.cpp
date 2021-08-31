#include "FMEmulator.h"

int main(int argc, char* argv[]) {
	if(argc == 2) {
		FileManagerEmulator fme;
		fme.ExecBatchFile(argv[1]);
		fme.Print();
	}
	else {
		std::cout << "Usage: FMEmulator.exe batch_file_path\n";
		system("pause");
	}
	return EXIT_SUCCESS;
}

