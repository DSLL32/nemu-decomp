unit RCP;

interface

uses UnitDecs;

{ These four typed-constant tables survive smart linking in Input.dll.
  Table names are inferred; entries, order, and array bounds come from the binary. }
const
  CPURegisterNames: array[0..31] of string = (
    'Index', 'Random', 'EntryLo0', 'EntryLo1',
    'Context', 'PageMask', 'Wired', 'reserved',
    'BadVAddr', 'Count', 'EntryHi', 'Compare',
    'Status', 'Cause', 'ExceptPC', 'reserved',
    'reserved', 'reserved', 'reserved', 'reserved',
    'PRevID', 'Config', 'LLAddr', 'WatchLo',
    'WatchHi', 'XContext', 'PErr', 'CacheErr',
    'TagLo', 'TagHi', 'ErrorEPC', 'reserved');

  RSPRegisterNames: array[0..31] of string = (
    'SP_MEM_ADDR_REG', 'SP_DRAM_ADDR_REG', 'SP_RD_LEN_REG', 'SP_WR_LEN_REG',
    'SP_STATUS_REG', 'SP_DMA_FULL_REG', 'SP_DMA_BUSY_REG', 'SP_SEMAPHORE_REG',
    'DPC_START_REG', 'DPC_END_REG', 'DPC_CURRENT_REG', 'DPC_STATUS_REG',
    'DPC_CLOCK_REG', 'DPC_BUFBUSY_REG', 'DPC_PIPEBUSY_REG', 'DPC_TMEM_REG',
    'reserved', 'reserved', 'reserved', 'reserved',
    'reserved', 'reserved', 'reserved', 'reserved',
    'reserved', 'reserved', 'reserved', 'reserved',
    'reserved', 'reserved', 'reserved', 'reserved');

  VectorControlNames: array[0..31] of string = (
    'VCO', 'VCC', 'VCE', 'reserved',
    'reserved', 'reserved', 'reserved', 'reserved',
    'reserved', 'reserved', 'reserved', 'reserved',
    'reserved', 'reserved', 'reserved', 'reserved',
    'reserved', 'reserved', 'reserved', 'reserved',
    'reserved', 'reserved', 'reserved', 'reserved',
    'reserved', 'reserved', 'reserved', 'reserved',
    'reserved', 'reserved', 'reserved', 'reserved');

  GeneralRegisterNames: array[0..31] of string = (
    'R0', 'AT', 'V0', 'V1',
    'A0', 'A1', 'A2', 'A3',
    'T0', 'T1', 'T2', 'T3',
    'T4', 'T5', 'T6', 'T7',
    'S0', 'S1', 'S2', 'S3',
    'S4', 'S5', 'S6', 'S7',
    'T8', 'T9', 'K0', 'K1',
    'GP', 'SP', 'S8', 'RA');

implementation

end.
