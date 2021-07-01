// Apache Thrift service IDL

enum PIO {
    OUT_IMAGING   = 0x00, INP_IMAGING   = 0x15, 
    OUT_SPECTRUM  = 0x60, INP_SPECTRUM  = 0x1A,
    OUT_SPEC_ARC  = 0XF0,  INP_SPEC_ARC  = 0x2A,
//  OUT_SPEC_ARC  = -16,  INP_SPEC_ARC  = 0x2A,
    OUT_LAMP_FLAT = 0x88, INP_LAMP_FLAT = 0x25,
    OUT_SKY_FLAT  = 0x00, INP_SKY_FLAT  = 0x15,
}

enum DeployState {
    UNK = -5,
    BAD = -4,
    ERR = -3,
    INV = -2,
    GET = -1,
    DIS =  0,
    ENA =  1,
}

enum DetectorState {
    UNK    = -5,
    BAD    = -4,
    ERR    = -3,
    INV    = -2,
    GET    = -1,
    IDLE   =  0,
    EXPOSE =  1,
    READOUT=  2,
}

enum FilterID {
    FILTER0 = 0,
    FILTER1 = 1,
}

enum FilterState {
    BAD  = -4,
    ERR  = -3,
    INV  = -2,
    GET  = -1,
    POS0 =  0,
    POS1 =  1,
    POS2 =  2,
    POS3 =  3,
    POS4 =  4,
    POS5 =  5,
}

enum FlatType {
    LAMP = 1,
    SKY  = 2,
    DOME = 3,
}

enum ReadRate {
    FAST = 0,
    SLOW = 1,
}

enum AcquireType { 
    WCS       = 0,
    BRIGHTEST = 1,
}

struct DetectorConfig {
    1: DetectorState State = DetectorState.UNK,
    2: double Exp  = 0.1,
    3: double Gain = 0.5,
    4: i32    Bin  = 1,
    5: i32    Rate = 0,
    6: FilterState Filter0 = FilterState.BAD,
    7: FilterState Filter1 = FilterState.BAD, 
    8: i32    Repeat = 1, 
    9: bool   Shutter= false,
   10: string RA  = "00:00:00.000",
   11: string DEC = "00:00:00.000",
   12: AcquireType Acquire = AcquireType.WCS,
}

struct FilterConfig {
    1: FilterState Filter0 = FilterState.BAD,
    2: FilterState Filter1 = FilterState.BAD, 
}

service InstSrv {
    DeployState  CtrlSlit   (1: DeployState state, 2: i32 timeout )
    DeployState  CtrlGrism  (1: DeployState state, 2: i32 timeout )
    DeployState  CtrlMirror (1: DeployState state, 2: i32 timeout )
    DeployState  CtrlLamp   (1: DeployState state )
    DeployState  CtrlArc    (1: DeployState state )
    DeployState  CtrlPIO    (1: i8 out, 2: i8 inp,  3: DeployState state, 4: i32 timeout )
    FilterState  CtrlFilter (1: FilterID filter,    2:FilterState state,  3: i32 timeout )
    FilterConfig CtrlFilters(1: FilterState state0, 2:FilterState state1, 3: i32 timeout )
}
