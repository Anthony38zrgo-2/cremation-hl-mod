#pragma once

#include "extdll.h"

// GoldSrc recycles edict addresses. Pair the pointer with the engine serial so
// cached owners, targets and private-data lookups cannot bind to a new entity.
struct EntityHandle {
    edict_t* entity=nullptr;
    int serial=0;

    EntityHandle()=default;
    explicit EntityHandle(edict_t* value) { assign(value); }

    void assign(edict_t* value) {
        entity=value;
        serial=value?value->serialnumber:0;
    }
    void clear() { entity=nullptr;serial=0; }
    edict_t* resolve() const {
        return entity&&!entity->free&&!(entity->v.flags&FL_KILLME)&&entity->serialnumber==serial
            ?entity:nullptr;
    }
};
