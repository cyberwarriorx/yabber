//
// Audio Overload
// Emulated music player
//
// (C) 2000-2007 Richard F. Bannister
//

//
// eng_protos.h
//

int32 ssf_start(uint8 *, uint32 length);
int32 ssf_gen(int16 *, uint32);
int32 ssf_stop(void);
int32 ssf_command(int32, int32);
int32 ssf_fill_info(ao_display_info *);
