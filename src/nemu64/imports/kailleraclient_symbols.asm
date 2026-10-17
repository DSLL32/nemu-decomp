; Import-library declarations, not Kaillera implementations.
;
; VC6 LIB /DEF recognizes a stdcall name as already decorated when it is
; present in a COFF symbol table. These absolute symbols provide that table
; without emitting code or data. LIB /DEF does not copy this object into
; its output import library.
.386
.model flat

PUBLIC _kailleraChatSend@4
PUBLIC _kailleraEndGame@0
PUBLIC _kailleraGetVersion@4
PUBLIC _kailleraInit@0
PUBLIC _kailleraModifyPlayValues@8
PUBLIC _kailleraSelectServerDialog@4
PUBLIC _kailleraSetInfos@4
PUBLIC _kailleraShutdown@0

_kailleraChatSend@4 EQU 0
_kailleraEndGame@0 EQU 0
_kailleraGetVersion@4 EQU 0
_kailleraInit@0 EQU 0
_kailleraModifyPlayValues@8 EQU 0
_kailleraSelectServerDialog@4 EQU 0
_kailleraSetInfos@4 EQU 0
_kailleraShutdown@0 EQU 0

END
