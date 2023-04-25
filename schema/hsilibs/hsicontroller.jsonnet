local moo = import "moo.jsonnet";
local ns = "dunedaq.hsilibs.hsicontroller";
local s = moo.oschema.schema(ns);

local s_app = import "appfwk/app.jsonnet";
local app = moo.oschema.hier(s_app).dunedaq.appfwk.app;

local cs = {
	
    str : s.string("Str", doc="A string field"),
    
    size: s.number("Size", "u8",
        doc="A count of very many things"),

    uint_data: s.number("UintData", "u4",
        doc="A count of very many things"),

    double_data: s.number("DoubleData", "f8", 
         doc="A double"),

    bool_data: s.boolean("BoolData", doc="A bool"),

    conf: s.record("ConfParams",[
        s.field("device", self.str, "",
            doc="String of managed device name"),
        s.field("hardware_state_recovery_enabled", self.bool_data, true,
            doc="control flag for hardware state recovery"),
        s.field("timing_session_name", self.str, "",
            doc="Name of managed device timing session"),
        s.field("clock_frequency", self.size,
            doc="HSI firmware clock frequency"),
        s.field("trigger_rate", self.double_data,
            doc="Trigger rate (in Hz) for the HSIEvent generation"),
        s.field("address", self.uint_data,
            doc="HSI endpoint address"),
        s.field("partition", self.uint_data,
            doc="HSI endpoint partition"),
        s.field("rising_edge_mask", self.uint_data,
            doc="Rising edge mask for HSI triggering"),
        s.field("falling_edge_mask", self.uint_data,
            doc="Falling edge mask for HSI triggering"),
        s.field("invert_edge_mask", self.uint_data,
            doc="Invert edge mask for HSI triggering"),
        s.field("data_source", self.uint_data,
            doc="Source of data for HSI triggering"),
    ], doc="Structure for payload of hsi configure commands"),
};

s_app + moo.oschema.sort_select(cs)