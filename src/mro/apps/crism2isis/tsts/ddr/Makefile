APPNAME = crism2isis

include $(ISISROOT)/make/isismake.tsts

commands:
	$(APPNAME) from=$(INPUT)/frt00003e25_01_de156l_ddr1.lbl \
	to=$(OUTPUT)/ddrtruth.cub \
	  | $(SED) 's+.*% Processed.*#+#+' \
	  | grep -v "Processed" \
	  | grep -v "Working" \
	  | grep -v "Importing" \
	  >& $(OUTPUT)/resultsLog.pvl;

