SECTIONS {
    rom = .;
    .code: {
        *(CODE);
    }
    .fill : {
        RESERVE(0x8000-.);
    } =0xffff
}
