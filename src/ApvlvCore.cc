/*
 * This file is part of the apvlv package
 *
 * Copyright (C) 2008 Alf.
 *
 * Contact: Alf <naihe2010@126.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2.0 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/* @CFILE ApvlvCore.cc
 *
 *  Author: Alf <naihe2010@126.com>
 */
/* @date Created: 2009/01/04 09:34:51 Alf*/

#include "ApvlvParams.h"
#include "ApvlvView.h"
#include "ApvlvCore.h"

#include <glib/gstdio.h>

#include <iostream>
#include <fstream>

namespace apvlv
{
class ApvlvView;
extern ApvlvView *gView;

ApvlvCore::ApvlvCore ()
{
  mInuse = true;

  mReady = false;

  mProCmd = 0;

  mRotatevalue = 0;

  mSearchResults = NULL;
  mSearchStr = "";

  mVbox = gtk_vbox_new (FALSE, 0);
  g_object_ref (mVbox);

  mScrollwin = gtk_scrolled_window_new (NULL, NULL);
  if (gParams->valueb ("scrollbar"))
    {
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (mScrollwin),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    }
  else
    {
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (mScrollwin),
                                      GTK_POLICY_NEVER, GTK_POLICY_NEVER);
    }

  mVaj =
    gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (mScrollwin));
  mHaj =
    gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (mScrollwin));
}

ApvlvCore::~ApvlvCore ()
{
  if (mGMonitor)
    {
      mGMonitor = NULL;
    }

  g_object_unref (mVbox);
}

bool ApvlvCore::reload ()
{
  return true;
}

void ApvlvCore::inuse (bool use)
{
  mInuse = use;

  if (mInuse == false && gView->hasloaded (filename (), type ()) == false)
    {
      debug ("core :%p is not needed, delete it\n", this);
      delete this;
    }
}

bool ApvlvCore::inuse ()
{
  return mInuse;
}

int ApvlvCore::type ()
{
  return mType;
}

returnType ApvlvCore::process (int has, int ct, guint key)
{
  return MATCH;
}

void ApvlvCore::setsize (int w, int h)
{
  gtk_widget_set_size_request (widget (), w, h);
  gtk_widget_set_size_request (mScrollwin, w, h - 16);
  mStatus->setsize (w, 15);
  mWidth = w;
  mHeight = h;
}

ApvlvCore *ApvlvCore::copy ()
{
  return NULL;
}

const char *ApvlvCore::filename ()
{
  return mFilestr.length () > 0 ? mFilestr.c_str () : NULL;
}

gint ApvlvCore::pagenumber ()
{
  return mPagenum + 1;
}

gint ApvlvCore::getrotate ()
{
  return mRotatevalue;
}

gdouble ApvlvCore::zoomvalue ()
{
  return mZoomrate;
}

ApvlvFile *ApvlvCore::file ()
{
  return mFile;
}

bool ApvlvCore::writefile (const char *name)
{
  if (mFile != NULL)
    {
      debug ("write %p to %s", mFile, name);
      return mFile->writefile (name ? name : filename ());
    }
  return false;
}

bool ApvlvCore::loadfile (const char *file, bool check)
{
  return false;
}

GtkWidget *ApvlvCore::widget ()
{
  return mVbox;
}

void ApvlvCore::showpage (gint p, gdouble s)
{
}

void ApvlvCore::refresh ()
{
}

double ApvlvCore::scrollrate ()
{
  double maxv = mVaj->upper - mVaj->lower - mVaj->page_size;
  double val = mVaj->value / maxv;
  if (val > 1.0)
    {
      return 1.00;
    }
  else if (val > 0.0)
    {
      return val;
    }
  else
    {
      return 0.00;
    }
}

gboolean ApvlvCore::scrollto (double s)
{
  if (!mReady)
    return FALSE;

  if (mVaj->upper != mVaj->lower)
    {
      double maxv = mVaj->upper - mVaj->lower - mVaj->page_size;
      double val = maxv * s;
      gtk_adjustment_set_value (mVaj, val);
      mStatus->show ();
      return TRUE;
    }
  else
    {
      debug ("fatal a timer error, try again!");
      return FALSE;
    }
}

void ApvlvCore::scrollup (int times)
{
  if (!mReady)
    return;

  gdouble val = gtk_adjustment_get_value (mVaj);
  gdouble sub = mVaj->upper - mVaj->lower;
  mVrate = sub / mLines;

  if (val - mVrate * times > mVaj->lower)
    {
      gtk_adjustment_set_value (mVaj, val - mVrate * times);
    }
  else if (val > mVaj->lower)
    {
      gtk_adjustment_set_value (mVaj, mVaj->lower);
    }
  else
    {
      if (gParams->valueb ("autoscrollpage"))
        {
          if (gParams->valueb ("continuous"))
            {
              showpage (mPagenum - 1,
                        ((mVaj->upper / 2) - mVrate * times) / (sub -
                            mVaj->page_size));
            }
          else
            {
              showpage (mPagenum - 1, 1.0);
            }
        }
    }

  mStatus->show ();
}

void ApvlvCore::scrolldown (int times)
{
  if (!mReady)
    return;

  gdouble val = gtk_adjustment_get_value (mVaj);
  gdouble sub = mVaj->upper - mVaj->lower;
  mVrate = sub / mLines;

  if (val + mVrate * times + mVaj->page_size < mVaj->upper)
    {
      gtk_adjustment_set_value (mVaj, val + mVrate * times);
    }
  else if (val + mVaj->page_size < mVaj->upper)
    {
      gtk_adjustment_set_value (mVaj, mVaj->upper - mVaj->page_size);
    }
  else
    {
      if (gParams->valueb ("autoscrollpage"))
        {
          if (gParams->valueb ("continuous"))
            {
              showpage (mPagenum + 1, (sub - mVaj->page_size) / 2 / sub);
            }
          else
            {
              showpage (mPagenum + 1, 0.0);
            }
        }
    }

  mStatus->show ();
}

void ApvlvCore::scrollleft (int times)
{
  if (!mReady)
    return;

  mHrate = (mHaj->upper - mHaj->lower) / mChars;
  gdouble val = mHaj->value - mHrate * times;
  if (val > mVaj->lower)
    {
      gtk_adjustment_set_value (mHaj, val);
    }
  else
    {
      gtk_adjustment_set_value (mHaj, mHaj->lower);
    }
}

void ApvlvCore::scrollright (int times)
{
  if (!mReady)
    return;

  mHrate = (mHaj->upper - mHaj->lower) / mChars;
  gdouble val = mHaj->value + mHrate * times;
  if (val + mHaj->page_size < mHaj->upper)
    {
      gtk_adjustment_set_value (mHaj, val);
    }
  else
    {
      gtk_adjustment_set_value (mHaj, mHaj->upper - mHaj->page_size);
    }
}

bool ApvlvCore::hascontent ()
{
  return false;
}

bool ApvlvCore::usecache ()
{
  return false;
}

void ApvlvCore::usecache (bool use)
{
}

bool ApvlvCore::print (int ct)
{
  return false;
}

bool ApvlvCore::totext (const char *name)
{
  return false;
}

bool ApvlvCore::rotate (int ct)
{
  return false;
}

void ApvlvCore::markposition (const char s)
{
}

void ApvlvCore::setzoom (const char *z)
{
}

void ApvlvCore::jump (const char s)
{
}

void ApvlvCore::nextpage (int times)
{
}

void ApvlvCore::prepage (int times)
{
}

void ApvlvCore::halfnextpage (int times)
{
}

void ApvlvCore::halfprepage (int times)
{
}

bool ApvlvCore::search (const char *str, bool reverse)
{
  return false;
}

bool ApvlvCore::continuous ()
{
  return false;
}

void ApvlvCore::gotolink (int ct)
{
}

int ApvlvCore::getskip ()
{
  return mSkip;
}

void ApvlvCore::setskip (int ct)
{
  mSkip = ct;
}

void ApvlvCore::returnlink (int ct)
{
}

void ApvlvCore::setactive (bool act)
{
  mActive = act;

  if (mActive && filename () && gView)
    {
      gchar *base = g_path_get_basename (filename ());
      gView->settitle (base);
      g_free (base);
    }
}

ApvlvCoreStatus::ApvlvCoreStatus ()
{
  mHbox = gtk_hbox_new (FALSE, 0);
}

ApvlvCoreStatus::~ApvlvCoreStatus ()
{
}

GtkWidget *ApvlvCoreStatus::widget ()
{
  return mHbox;
}

void ApvlvCoreStatus::active (bool act)
{
}

void ApvlvCoreStatus::setsize (int w, int h)
{
}

void ApvlvCoreStatus::show ()
{
}
}