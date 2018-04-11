window.optimizely.push({type:"load",data:{changes:[{"dependencies": [], "type": "custom_code", "id": "DA8ED781-30EA-46E8-AF54-2BB97ED6C50B", "value": function($){(function ($) {
    var utils = window.optimizely.get('utils'),
        path = window.location.href;
    if ($ !== undefined) {
        //Replace Health bin with AR_58
        utils.waitForElement('#homepage3-zone-1 .zn__containers .zn__column--idx-5 .cn--idx-5 .cd--idx-0').then(function (outbrainElem) {
            replaceOutbrain($(outbrainElem), '#homepage3-zone-1 .zn__containers .zn__column--idx-5 .cn--idx-5 .cd--idx-0', 'OB_AR_58', 'AR_58');
          $("#homepage3-zone-1 .zn__containers .zn__column--idx-5 .cn--idx-5 .cd--idx-1").remove();
          $("#homepage3-zone-1 .zn__containers .zn__column--idx-5 .cn--idx-5 .cd--idx-2").remove();
        });

        // function to replace the outbrain widgets
        var replaceOutbrain = function(section, widgetId, newId, dataWidgetId) {
            section.replaceWith('<div class="' + newId + '"><div class="OUTBRAIN" data-src="' + path + '" data-widget-id="' + dataWidgetId + '" data-ob-template="cnn" ></div></div>');
        };
    }


}(window.jQuery));
}}]}});