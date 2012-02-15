#!/bin/sh
#
# Prepares a release:
#   - Bumps version according to specified level (major, minor, or patch)
#   - Updates all necessary headers with new version
#   - Commits the changes
#   - Tags the release
#   - Creates a release tarball

PROJECT=spectrwm
PROJECT_UC=$(echo $PROJECT | tr '[:lower:]' '[:upper:]')
SCRIPT=$(basename $0)
HEADER=version.h

# verify params
if [ $# -lt 1 ]; then
	echo "usage: $SCRIPT {major | minor | patch}"
	exit 1
fi

report_err()
{
	echo "$SCRIPT: error: $1" 1>&2
	exit 1
}


cd "$(dirname $0)"

# verify header exists
if [ ! -f "$HEADER" ]; then
	report_err "$HEADER does not exist"
fi

# verify valid release type
RTYPE="$1"
if [ "$RTYPE" != "major" -a "$RTYPE" != "minor" -a "$RTYPE" != "patch" ]; then
	report_err "release type must be major, minor, or patch"
fi

# verify git is available
if ! type git >/dev/null 2>&1; then
	report_err "unable to find 'git' in the system path"
fi

# verify the git repository is on the master branch
BRANCH=$(git branch | grep '\*' | cut -c3-)
if [ "$BRANCH" != "master" ]; then
	report_err "git repository must be on the master branch"
fi

# verify there are no uncommitted modifications prior to release modifications
NUM_MODIFIED=$(git diff 2>/dev/null | wc -l | sed 's/^[ \t]*//')
NUM_STAGED=$(git diff --cached 2>/dev/null | wc -l | sed 's/^[ \t]*//')
if [ "$NUM_MODIFIED" != "0" -o "$NUM_STAGED" != "0" ]; then
	report_err "the working directory contains uncommitted modifications"
fi

# get version
PAT_PREFIX="(^#define[[:space:]]+${PROJECT_UC}"
PAT_SUFFIX='[[:space:]]+)[0-9]+$'
MAJOR=$(egrep "${PAT_PREFIX}_MAJOR${PAT_SUFFIX}" $HEADER | awk '{print $3}')
MINOR=$(egrep "${PAT_PREFIX}_MINOR${PAT_SUFFIX}" $HEADER | awk '{print $3}')
PATCH=$(egrep "${PAT_PREFIX}_PATCH${PAT_SUFFIX}" $HEADER | awk '{print $3}')
if [ -z "$MAJOR" -o -z "$MINOR" -o -z "$PATCH" ]; then
	report_err "unable to get version from $HEADER"
fi

# bump version according to level
if [ "$RTYPE" = "major" ]; then
	MAJOR=$(expr $MAJOR + 1)
	MINOR=0
	PATCH=0
elif [ "$RTYPE" = "minor" ]; then
	MINOR=$(expr $MINOR + 1)
	PATCH=0
elif [ "$RTYPE" = "patch" ]; then
	PATCH=$(expr $PATCH + 1)
fi
PROJ_VER="$MAJOR.$MINOR.$PATCH"

# update header with new version
sed -E "
    s/${PAT_PREFIX}_MAJOR${PAT_SUFFIX}/\1${MAJOR}/;
    s/${PAT_PREFIX}_MINOR${PAT_SUFFIX}/\1${MINOR}/;
    s/${PAT_PREFIX}_PATCH${PAT_SUFFIX}/\1${PATCH}/;
" <"$HEADER" >"${HEADER}.tmp"

# apply changes
mv "${HEADER}.tmp" "$HEADER"

# commit and tag
TAG="${PROJECT_UC}_${MAJOR}_${MINOR}_${PATCH}"
git commit -am "Prepare for release ${PROJ_VER}." ||
    report_err "unable to commit changes"
git tag -a "$TAG" -m "Release ${PROJ_VER}" || report_err "unable to create tag"

# create temp working space and copy repo over
TD=$(mktemp -d /tmp/release.XXXXXXXXXX)
if [ ! -d "$TD" ]; then
	report_err "unable to create temp directory"
fi
RELEASE_DIR="$PROJECT-$PROJ_VER"
RELEASE_TAR="$PROJECT-$PROJ_VER.tgz"
git clone . "$TD/$RELEASE_DIR" ||
    report_err "unable to copy to $TD/$RELEASE_DIR"

# cleanup repository files
cd "$TD"
if [ -d "$RELEASE_DIR" -a -d "$RELEASE_DIR/.git" ]; then
        rm -rf "$RELEASE_DIR/.git"
fi
if [ -d "$RELEASE_DIR" -a -f "$RELEASE_DIR/.gitignore" ]; then
        rm -f "$RELEASE_DIR/.gitignore"
fi

# make snap
tar -zcf "$RELEASE_TAR" "$RELEASE_DIR" ||
    report_err "unable to create $RELEASE_TAR"


echo "Release tarball:"
echo "  $TD/$RELEASE_TAR"
echo ""
echo "If everything is accurate, use the following command to push the changes:"
echo "  git push --tags origin master"
