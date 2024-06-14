local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.hsilibs.fakehsieventgeneratorinfo");

local info = {
   cl : s.string("class_s", moo.re.ident,
                  doc="A string field"), 
    uint8  : s.number("uint8", "u8",
                     doc="An unsigned of 8 bytes"),

    //counter_vector: s.sequence("HwCommandCounters", self.uint8,
    //        doc="A vector hardware command counters"),

   info: s.record("Info", [
       s.field("generated_hsi_events_counter", self.uint8, doc="Number of generated HSIEvents so far"), 
       s.field("sent_hsi_events_counter", self.uint8, doc="Number of sent HSIEvents so far"), 
       s.field("failed_to_send_hsi_events_counter", self.uint8, doc="Number of failed send attempts so far"), 
       s.field("last_generated_timestamp", self.uint8, doc="Timestamp of the last generated HSIEvent"), 
       s.field("last_sent_timestamp", self.uint8, doc="Timestamp of the last sent HSIEvent"), 
   ], doc="FakeHSIEventGeneratorModuleInfo information")
};

moo.oschema.sort_select(info)