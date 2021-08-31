#include "UnitConverter.h"

int main(int argc, char* argv[]) {
	if (3 == argc) {
		UnitConverter uconv;
		uconv.ReadInput(argv[1]);
		uconv.MakeOutput(argv[2]);
	}
	else {
		std::cout << "Usage: UnitConverter.exe input_file_path output_file_path\n";
		system("pause");
	}
	return EXIT_SUCCESS;
}