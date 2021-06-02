// Apache Thrift Hello World service IDL

enum DeployState {
    UNK = -5
    BAD = -4
    ERR = -3,
    INV = -2,
    GET = -1,
    DIS =  0,
    ENA =  1,
}

enum DetectorState {
    UNK    = -5
    BAD    = -4
    ERR    = -3,
    INV    = -2,
    GET    = -1,
    IDLE   =  0,
    EXPOSE =  1,
    READOUT=  2,
}

enum FilterID {
    Filter0 = 0,
    Filter1 = 1,
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
    DeployState  CtrlPIO    (1: i8 out, 2: i8 inp, 3: i32 timeout )
    FilterState  CtrlFilter (1: FilterID filter, 2: FilterState state   )
    FilterConfig CtrlFilters(1: FilterConfig config, 2: i32 timeout_ms )
}
