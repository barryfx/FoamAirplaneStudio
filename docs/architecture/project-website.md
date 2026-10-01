# Project website

The project landing page is maintained as part of the repository baseline in
`website/index.html` and `website/style.css`. It is a static HTML/CSS site with
no JavaScript runtime, analytics, external fonts or build dependencies.

The page includes the project's purpose statement, testing notice, feature
overview, example-project locations and three versioned installer links. The
top Downloads link targets the bottom `#downloads` section. Release files remain
GitHub Release assets; installers are not duplicated into the website artifact.

`.github/workflows/pages.yml` publishes to GitHub Pages on matching changes to
`main`, or via workflow_dispatch. Only the two site files and existing application
artwork are staged; repository documentation and other private files are not
implicitly published as site content. The repository was made public with the
owner's explicit approval so Pages and Release downloads are publicly accessible.

Site URL: https://barryfx.github.io/FoamAirplaneStudio/

For a new release, update the displayed version, installer filenames and release
links together. Keep platform testing statements aligned with README and the
validation records. Preserve the notice that no airplanes have been milled.

To preview, copy `website/index.html`, `website/style.css`, and
`resources/graphics/FoamAirplaneStudio.png` into `build/site-preview`, then run
`python -m http.server 8766 --bind 127.0.0.1 --directory build/site-preview`.
Open http://127.0.0.1:8766/. The image is labeled as concept/application artwork,
not evidence of a manufactured airplane.

Validation includes desktop and narrow-layout browser inspection, anchor and
overflow checks, and checking all three download URLs against release metadata.
The site is independent of model generation and installer builds.
