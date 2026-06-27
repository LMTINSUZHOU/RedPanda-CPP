#include <fstream>

int main(int argc, char *argv[])
{
    if (argc < 4)
        return 1;

    std::ifstream output(argv[2]);
    std::ifstream answer(argv[3]);
    int value = 0;
    int low = 0;
    int high = 0;
    if (!(output >> value))
        return 1;
    if (!(answer >> low >> high))
        return 1;

    return value >= low && value <= high ? 0 : 1;
}
