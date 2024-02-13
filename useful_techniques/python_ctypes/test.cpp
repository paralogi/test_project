#include <cstdio>

struct Resource {
    char model_hash[ 32 ];
    char model_file[ 256 ];
    double model_range;
};

Resource resource;

extern "C" Resource* globalResource() {
    sprintf( resource.model_hash, "test" );
    sprintf( resource.model_file, "test" );
    resource.model_range = 42;
    return &resource;
}
