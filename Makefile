TARGET_ISA=x86

GEM5_HOME=$(HOME)/gem5-Okapi
$(info GEM5_HOME is $(GEM5_HOME))

CXX=g++

CFLAGS=-I$(GEM5_HOME)/include
LDFLAGS=-L$(GEM5_HOME)/util/m5/build/$(TARGET_ISA)/out -lm5

OUTDIR=bin

all: $(OUTDIR)/m5_sum $(OUTDIR)/m5_lhl_se $(OUTDIR)/m5_timing $(OUTDIR)/m5_evic

$(OUTDIR)/m5_sum: m5_sum_testing.c | $(OUTDIR)
	$(CXX) -o $@ $< $(CFLAGS) $(LDFLAGS)

$(OUTDIR)/m5_lhl_se: m5_hit_level_SE.c | $(OUTDIR)
	$(CXX) -o $@ $< $(CFLAGS) $(LDFLAGS)

$(OUTDIR)/m5_timing: timing_test.c timing.c | $(OUTDIR)
	$(CC) -O2 -o $@ timing_test.c timing.c $(CFLAGS) $(LDFLAGS)

$(OUTDIR)/m5_evic: m5_evic.c threshold_group_testing.c address_set_adapter.c | $(OUTDIR)
	$(CC) -O2 -o $@ m5_evic.c threshold_group_testing.c address_set_adapter.c $(CFLAGS) $(LDFLAGS)

$(OUTDIR):
	mkdir -p $(OUTDIR)

clean:
	rm -rf $(OUTDIR)
