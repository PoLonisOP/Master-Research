#include "memory.hpp"

void Memory::Set_trace_files() {
    trace_fout.open(basefilename);
}

void Memory::Set_trace_files2() {
    trace_fout2.open(basefilename2);
}

void Memory::Trace_W() {
    pos = trace_fout.tellp();
    trace_fout.seekp(pos,ios::beg);
    Request_Arrival_Time += 10;
    Starting_Logical_Sector_Address += 8;
    write_times++;
    trace_fout << Request_Arrival_Time << " " << Device_Number << " " << Starting_Logical_Sector_Address << " " << Request_Size_In_Sectors << " " << 0 << endl;
}

void Memory::Trace_R() {
    pos = trace_fout.tellp();
    trace_fout.seekp(pos,ios::beg);
    Request_Arrival_Time += 10;
    Starting_Logical_Sector_Address += 8;
    read_times++;
    trace_fout << Request_Arrival_Time << " " << Device_Number << " " << Starting_Logical_Sector_Address << " " << Request_Size_In_Sectors << " " << 1 << endl;
}

void Memory::Trace_W2() {
    pos = trace_fout2.tellp();
    trace_fout2.seekp(pos,ios::beg);
    Request_Arrival_Time_2 += 10;
    Starting_Logical_Sector_Address_2 += 8;
    write_times_2++;
    trace_fout2 << Request_Arrival_Time_2 << " " << Device_Number << " " << Starting_Logical_Sector_Address_2 << " " << Request_Size_In_Sectors << " " << 0 << endl;
}

void Memory::Trace_R2() {
    pos = trace_fout2.tellp();
    trace_fout2.seekp(pos,ios::beg);
    Request_Arrival_Time_2 += 10;
    Starting_Logical_Sector_Address_2 += 8;
    read_times_2++;
    trace_fout2 << Request_Arrival_Time_2 << " " << Device_Number << " " << Starting_Logical_Sector_Address_2 << " " << Request_Size_In_Sectors << " " << 1 << endl;
}