/* exported srchrep */

var srchrep = (function () {
    var savedText = "";

    function saveText() {
        savedText = opener.parent.docRef.editform.text_data.value;
    }

    function escapeRegExp(string) {
        return string.replace(/[.*+?^${}()|[\]\\]/g, "\\$&"); // $& means the whole matched string
    }

    function setUndoButtonDisabled(state) {
        document.getElementById("undo").disabled = state;
    }

    function doReplace() {
        var search = document.getElementById("search").value;
        var replacetext = document.getElementById("replace").value.replace(new RegExp("\\\\n", "g"), "\r\n");
        saveText();
        if (!document.getElementById("is_regex").checked) {
            search = escapeRegExp(search);

            // Replace $ in replace text with $$ to avoid special replacement patterns.
            // Note, $ here is in a .replace function and must be escaped so '$$' becomes '$$$$'.
            replacetext = replacetext.replace(/\$/g, "$$$$");
        }
        opener.parent.docRef.editform.text_data.value = opener.parent.docRef.editform.text_data.value.replace(new RegExp(search, "gum"), replacetext);
        setUndoButtonDisabled(false);
    }

    function restoreSavedText() {
        opener.parent.docRef.editform.text_data.value = savedText;
        setUndoButtonDisabled(true);
    }

    return {
        doReplace: doReplace,
        restoreSavedText: restoreSavedText,
    };
})();
