
extern "C" {

    extern u32 __start__;
    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;


void __libnx_initheap(void) {
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

    void __appInit(void) {

    }

    void __appExit(void) {

    }

}

int main(int argc, char **argv) {

    return 0;
}
