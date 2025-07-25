<?php
// This file sets up the common infrastructure for the site. It is used for
// both HTML pages (via base.inc) and suitable for API calls as well.
// This file should only include functionality that is common in both.
//
// This file sets up the vital infrastructure for the site, including:
// * including site_vars.php to set global variables
// * registering uncaught exception handler
// * connecting to the database
// * pulling in gettext functions
//
// The order of the include_once()s in this file is important!

// Override $relPath to be the absolute path to the directory that
// contains this file. This allows registered shutdown functions
// to know where to find files (working directory of the script
// can change inside the shutdown function based on PHP docs).
// This also helps things like APC which can bypass stat calls
// on includes that use absolute paths.
$relPath = dirname(__FILE__)."/";

// store the current time to calculate total page build time
$PAGE_START_TIME = microtime(true);

require_once $relPath.'SiteConfig.inc';

// Register autoloader
spl_autoload_register('dp_class_autoloader');

// Load composer autoloader
require_once($relPath.'../vendor/autoload.php');

// Exception handlers are defined differently for HTML and the API
set_exception_handler(SiteConfig::get()->testing ? 'test_exception_handler' : 'production_exception_handler');

if (!headers_sent()) {
    // Tell proxies to vary the caching based on the Accept-Language header
    // to ensure others behind the proxy don't inadvertently get a language
    // they aren't expecting.
    header("Vary: Accept-Language");
}

include_once($relPath.'gettext_setup.inc');
include_once($relPath.'DPDatabase.inc');

if (!defined('SKIP_DB_CONNECT')) {
    try {
        DPDatabase::connect();
    } catch (Exception $e) {
        // If we're in maintenance mode, don't die here - we'll more gracefully
        // error out later
        if (!SiteConfig::get()->maintenance) {
            throw $e;
        }
    }
}

// Load the session libraries, but allow base.inc and api/index.php to do
// the start/resume.
include_once($relPath.'dpsession.inc');

// Include misc.inc which contains useful functions that almost 90% of
// the pages use in some way.
include_once($relPath.'misc.inc');

//----------------------------------------------------------------------------

// Autoloading functions for DP classes in pinc/
function dp_class_autoloader($class)
{
    global $relPath;
    if (is_file($relPath.$class.".inc")) {
        include_once($relPath.$class.".inc");
    }
}

function dp_setcookie($name, $value = "", $expires = 0, $options = [])
{
    $defaults = [
        "expires" => $expires,
        "path" => "/",
        "secure" => SiteConfig::get()->use_secure_cookies,
        "samesite" => "Lax",
    ];

    $options = array_merge($defaults, $options);
    return setcookie($name, $value, $options);
}
