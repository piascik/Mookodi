// Apache Thrift Hello World service IDL

enum DeployState {
    GET = 0,
    ENA = 1,
    DIS = 2,
    UNK = 3,
    ERR = 4,
    INV = 5,
}

enum DetectorState {
    GET    = 0,
    IDLE   = 1,
    EXPOSE = 2,
    READOUT= 3,
    UNK    = 4,
    ERR    = 5,
    INV    = 6,
}

enum FilterState {
    GET  = 0,
    POS1 = 1,
    POS2 = 2,
    POS3 = 3,
    POS4 = 4,
    POS5 = 5,
    POS6 = 6,
    UNK  = 7,
    ERR  = 8,
    INV  = 9,
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
    6: FilterState Filter1 = FilterState.UNK,
    7: FilterState Filter2 = FilterState.UNK, 
    8: i32    Repeat = 1, 
    9: bool   Shutter= false,
   10: string RA  = "00:00:00.000",
   11: string DEC = "00:00:00.000",
   12: AcquireType Acquire = AcquireType.WCS,
}

service InstSrv {
    string CtrlSlit  (1: DeployState state )
    string CtrlGrism (1: DeployState state )
    string CtrlMirror(1: DeployState state )
    string CtrlLamp  (1: DeployState state )
    string CtrlArc   (1: DeployState state )
    DetectorConfig   CtrlDetector(1: DetectorConfig config )
    oneway void      CtrlDetectOW(1: DetectorConfig config )
}
