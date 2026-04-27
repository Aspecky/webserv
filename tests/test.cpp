#include "lest.hpp"

extern lest::tests rulesSpec;
extern lest::tests parserSpec;

int main(int argc, char* argv[])
{
    lest::tests spec;
    spec.insert(spec.end(), rulesSpec.begin(), rulesSpec.end());
    spec.insert(spec.end(), parserSpec.begin(), parserSpec.end());
    return lest::run(spec, argc, argv);
}
