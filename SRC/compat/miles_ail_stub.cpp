// Mig Alley Linux port — Miles Sound System (AIL) stubs.
// The real Miles lib (mss32) is Windows-only; these no-op C-linkage stubs let
// the game link and run silent. Replace with OpenAL/SDL_mixer audio later.
// All AIL_* are cdecl C-linkage: caller cleans the stack, so a zero-arg stub
// returning 0 satisfies any call site regardless of its declared signature.
// NOTE: the DIGITAL-SAMPLE subset (startup/waveOutOpen, sample handle alloc/init/
// release, set_sample_file/address/type, start/stop/resume/end, volume/pan/rate/loop,
// status/position, digital master volume, shutdown, last_error) is now implemented for
// real on OpenAL in SRC/compat/ma_openal.cpp -- removed from here to avoid duplicate
// symbols. What remains below are the still-silent MIDI/XMIDI music + DLS + sequence +
// timer/lock stubs (a later audio increment).
#if defined(MA_LINUX) || defined(FF_LINUX)
extern "C" {
long AIL_allocate_sequence_handle(){return 0;}
long AIL_branch_index(){return 0;}
long AIL_create_wave_synthesizer(){return 0;}
long AIL_destroy_wave_synthesizer(){return 0;}
long AIL_DLS_close(){return 0;}
long AIL_DLS_load_memory(){return 0;}
long AIL_DLS_open(){return 0;}
long AIL_end_sequence(){return 0;}
long AIL_extract_DLS(){return 0;}
long AIL_file_type(){return 0;}
long AIL_find_DLS(){return 0;}
long AIL_init_sequence(){return 0;}
long AIL_lock(){return 0;}
long AIL_midiOutClose(){return 0;}
long AIL_midiOutOpen(){return 0;}
long AIL_register_event_callback(){return 0;}
long AIL_register_trigger_callback(){return 0;}
long AIL_release_sequence_handle(){return 0;}
long AIL_release_timer_handle(){return 0;}
long AIL_resume_sequence(){return 0;}
long AIL_sequence_status(){return 0;}
long AIL_sequence_volume(){return 0;}
long AIL_set_DirectSound_HWND(){return 0;}
long AIL_set_sequence_volume(){return 0;}
long AIL_set_XMIDI_master_volume(){return 0;}
long AIL_start_sequence(){return 0;}
long AIL_stop_sequence(){return 0;}
long AIL_unlock(){return 0;}
}
#endif
