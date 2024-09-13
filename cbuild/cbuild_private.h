#ifndef CBUILD_PRIVATE_H
#define CBUILD_PRIVATE_H

#include "cbuild.h"

CError cbuild_create(CBuildType btype, CBuild *out_cbuild);

CError cbuild_build(CBuild *self);

void cbuild_destroy(CBuild *self);

#endif // CBUILD_PRIVATE_H
