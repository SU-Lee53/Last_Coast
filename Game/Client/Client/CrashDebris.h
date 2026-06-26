#pragma once
#include "NodeObject.h"

// Wreckage left at the helicopter crash site after the cinematic ends.
// Identical to NodeObject, but registered as a dynamic spatial object so it goes
// through the normal render pipeline (frustum culling / shadows / lighting),
// exactly like zombies -- not the cinematic prop's direct-render path.
class CrashDebris : public NodeObject {
public:
	CrashDebris() = default;
};
