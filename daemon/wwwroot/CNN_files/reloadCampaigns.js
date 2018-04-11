//<script>
bouncex.tryCatch(function reloadCampaigns(){
	var new_campaigns = false;
	bouncex.brandStyles = false;
	bouncex.webfonts = false;
	bouncex.gbi.stacks = false
	
	var campaign_added = false;
	for(var ca_id in new_campaigns){
		if(new_campaigns.hasOwnProperty(ca_id)){
			if(!bouncex.cookie.campaigns){
				bouncex.cookie.campaigns = {};
			}
			if(!bouncex.cookie.campaigns[ca_id]){
				campaign_added = true;
				bouncex.cookie.campaigns[ca_id] = {lvt:bouncex.cookie.lvt, vv:0};
			}
		}
	}
	if(campaign_added){
		bouncex.setBounceCookie();
	}
	
	for(var ca_id in bouncex.campaigns){
		if(bouncex.campaigns.hasOwnProperty(ca_id)){//copy state vars
			if(new_campaigns[ca_id]){
				new_campaigns[ca_id].ap = bouncex.campaigns[ca_id].ap;
				new_campaigns[ca_id].repressed = Boolean(bouncex.campaigns[ca_id].repressed);
			}
			if(new_campaigns[ca_id]&&
				bouncex.campaigns[ca_id].ad_visible&&
				new_campaigns[ca_id].html.replace(/fsa=(\d+)&|width=(\d+)&|height=(\d+)&/gi,'')==bouncex.campaigns[ca_id].html.replace(/fsa=(\d+)&|width=(\d+)&|height=(\d+)&/gi,'')){
								new_campaigns[ca_id] = bouncex.campaigns[ca_id];

			}else{
				bouncex.destroy_ad(ca_id);
			}
		}
	}

	bouncex.campaigns = new_campaigns;
	new_campaigns = {};
	
	bouncex.debug = false;
		bouncex.setBounceCookie();
	if (bouncex.campaigns) {
		bouncex.loadBounceCss(bouncex.initActivationFuncs);
		for (var ca_id in bouncex.campaigns) {
			if (bouncex.campaigns[ca_id].ad_visible && typeof bouncex.repressCampaigns === 'function') {
				bouncex.repressCampaigns(ca_id);
			}
		}
	}
		bouncex.loadBrandStyles();
	bouncex.loadWebfonts();

	})();
