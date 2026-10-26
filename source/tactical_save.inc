// The base HL entvars save list omits user fields. Write a named extension
// through the donor's own serializer instead of depending on private layouts.
static bool tacticalSaved(edict_t* e){return czIs(e)||!std::strcmp(text(e->v.targetname),"dmc_player_state")||!std::strcmp(text(e->v.targetname),"cz_planted_bomb");}
static TYPEDESCRIPTION tacticalFields[]={
    DEFINE_ENTITY_FIELD(iuser1,FIELD_INTEGER),DEFINE_ENTITY_FIELD(iuser2,FIELD_INTEGER),DEFINE_ENTITY_FIELD(iuser3,FIELD_INTEGER),DEFINE_ENTITY_FIELD(iuser4,FIELD_INTEGER),
    DEFINE_ENTITY_FIELD(fuser1,FIELD_TIME),DEFINE_ENTITY_FIELD(fuser2,FIELD_TIME),DEFINE_ENTITY_FIELD(fuser3,FIELD_TIME),DEFINE_ENTITY_FIELD(fuser4,FIELD_TIME),
    DEFINE_ENTITY_FIELD(vuser1,FIELD_VECTOR),DEFINE_ENTITY_FIELD(vuser2,FIELD_VECTOR),DEFINE_ENTITY_FIELD(vuser3,FIELD_VECTOR),DEFINE_ENTITY_FIELD(vuser4,FIELD_VECTOR)
};
