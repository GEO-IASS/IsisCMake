APPNAME = cisscal

include $(ISISROOT)/make/isismake.tsts


commands:
	$(APPNAME) from=$(INPUT)/W1294561261_1.c2i.nospice.cub \
	to=$(OUTPUT)/W1294561261_1.truth.cub \
	units="INTENSITY" \
	> /dev/null;
	catlab from=$(OUTPUT)/W1294561261_1.truth.cub to=$(OUTPUT)/labels.truth.pvl > /dev/null;
