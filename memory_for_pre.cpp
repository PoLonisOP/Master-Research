#include "memory_for_pre.hpp"

void Memory_Pre::Set_trace_files() {
    trace_fout.open(basefilename);
}

void Memory_Pre::Trace_W() {
    pos = trace_fout.tellp();
    trace_fout.seekp(pos,ios::beg);
    Request_Arrival_Time += 10;
    Starting_Logical_Sector_Address += 8;
    trace_fout << Request_Arrival_Time << " " << Device_Number << " " << Starting_Logical_Sector_Address << " " << Request_Size_In_Sectors << " " << 0 << endl;
}

void Memory_Pre::Trace_R() {
    pos = trace_fout.tellp();
    trace_fout.seekp(pos,ios::beg);
    Request_Arrival_Time += 10;
    Starting_Logical_Sector_Address += 8;
    trace_fout << Request_Arrival_Time << " " << Device_Number << " " << Starting_Logical_Sector_Address << " " << Request_Size_In_Sectors << " " << 1 << endl;
}