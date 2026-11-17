if(pixelSelected){
 bool selected;{std::lock_guard<std::mutex> lock(mutex);selected=CurrentLocked(seen,held)&&active;pixelDrawn=false;pixel={};pixel.epoch=seen;pixelFailed=!selected;status="{\"state\":\"DIAGNOSTIC_WAIT_PAGE_PULSE\",\"evidenceOrigin\":\"DIAGNOSTIC_NOT_PHYSICS\"}";}
 if(!selected)failed=true;
 continue;
}
