TARGET_ISA=x86

GEM5_HOME=$(HOME)/gem5-Okapi
$(info GEM5_HOME is $(GEM5_HOME))

CXX=g++

CFLAGS=-I$(GEM5_HOME)/include
LDFLAGS=-L$(GEM5_HOME)/util/m5/build/$(TARGET_ISA)/out -lm5

OUTDIR=bin

all: $(OUTDIR)/m5_sum $(OUTDIR)/m5_lhl_se

$(OUTDIR)/m5_sum: m5_sum_testing.c | $(OUTDIR)
	$(CXX) -o $@ $< $(CFLAGS) $(LDFLAGS)

$(OUTDIR)/m5_lhl_se: m5_hit_level_SE.c | $(OUTDIR)
	$(CXX) -o $@ $< $(CFLAGS) $(LDFLAGS)

$(OUTDIR):
	mkdir -p $(OUTDIR)

clean:
	rm -rf $(OUTDIR)
