function openNav() {	
	if(document.getElementById("sidebar").style.width == "250px")
	{
		document.getElementById("sidebar").style.width = "0";
    		document.getElementById("mainpart").style.marginLeft= "0";
	}
	else
	{
		document.getElementById("sidebar").style.width = "250px";
		document.getElementById("mainpart").style.marginLeft = "250px";
	}
}
