   class Control {
#+
#  Name:
#     Control

#  Type of Module:
#     [incr Tk] Mega-Widget

#  Purpose:
#     Base class for viewer controls.

#  Description:
#     This class provides a base from which specific controls can inherit.  
#     Such controls will have a compact representation (e.g. a button) 
#     which can be placed on a control bar, and will allow the user to
#     modify a given value or set of values.  It may need to pop up
#     additional windows for user interaction if a button-sized area
#     is not big enough for it.

#  Public Methods:
#

#  Public Procedures:
#
#     max num ...
#        Gives the maximum of all the (numeric) arguments supplied.
#        Just here because it's useful.
#
#     mim num ...
#        Gives the minimum of all the (numeric) arguments supplied.
#        Just here because it's useful.
#

#  Public Variables (Configuration Options):
#
#     balloonstr
#        A short which will pop up in a balloon if the cursor stays over
#        the widget for longer than a short space of time.
#
#     childsite
#        The pathname of the childsite widget, in which the specific 
#        control should be put.
#
#     command
#        This gives a command which will be executed whenever the value
#        of the control changes.  Any occurrence of the sequence "%v" 
#        in the command variable will be replaced by the value of the
#        variable before it is executed.
#
#     hidden
#        This variable has a boolean value which indicates whether it is
#        visible or not.  By default it is not hidden, but if this is
#        configured true, it will effectively be invisible.
#
#     label
#        The label displayed to the user to indicate what the widget is
#        here for.  If it is a button, this will be the label on the
#        button, but for other widget kinds it may be presented in
#        other ways.
#
#     state
#        This variable may have one of the values "normal" or "disabled".
#        If "disabled" no user interaction will be permitted and the value
#        in the valuevar variable will not change.
#
#     valuevar
#        Gives the name of a variable which will behave as a public 
#        variable.  If the widget is configured with -valuevar name,
#        then the -name option may be used in cget or configure method
#        calls.
#
#        The coding of this is rather ugly and fragile.  This option 
#        should only be used if the control being used to compose a 
#        Mega-Widget.  You'd think there was some clean way round
#        this using Itk's complicated option handling facilities, but
#        it doesn't seem to work properly.
#        
#     value
#        The value of the widget.

#-

#  Inheritance:
      inherit iwidgets::Labeledwidget


########################################################################
#  Constructor.
########################################################################
      constructor { args } {

         itk_component add toolbar {
            iwidgets::toolbar $itk_interior.tbar
         } {
            usual
            ignore -selectborderwidth  ;# Don't know why I need this, but I do.
         }
         set childsite [ $itk_component(toolbar) add frame onlytool ]
         set tool $childsite
         set control $itk_component(toolbar)
         pack $control -side left -fill y -expand 1

         eval itk_initialize $args
      }


########################################################################
#  Public methods.
########################################################################

#-----------------------------------------------------------------------
      public method childsite { } {
#-----------------------------------------------------------------------
         return $childsite
      }

########################################################################
#  Public procedures.
########################################################################

#-----------------------------------------------------------------------
      public proc max { num args } {
#-----------------------------------------------------------------------
         foreach x $args {
            if { $x > $num } { set num $x }
         }
         return $num
      }


#-----------------------------------------------------------------------
      public proc min { num args } {
#-----------------------------------------------------------------------
         foreach x $args {
            if { $x < $num } { set num $x }
         }
         return $num
      }


########################################################################
#  Public variables.
########################################################################

#-----------------------------------------------------------------------
      public variable balloonstr {} {
#-----------------------------------------------------------------------
         $itk_component(toolbar) itemconfigure 0 -balloonstr $balloonstr
      }


#-----------------------------------------------------------------------
      public variable command {} {
#-----------------------------------------------------------------------
      }


#-----------------------------------------------------------------------
      public variable hidden {0} {
#-----------------------------------------------------------------------
         if { $hidden && ! [ catch { pack info $control } ] } {
            pack forget $control
         } elseif { ! $hidden && [ catch { pack info $control } ] } {
            pack $control
         }
      }


#-----------------------------------------------------------------------
      public variable label {} {
#-----------------------------------------------------------------------
      }


#-----------------------------------------------------------------------
      public variable value {} {
#-----------------------------------------------------------------------
         if { $valuecmd != "" } {
            if { [ catch { eval $valuecmd "{$value}" } ] } {
               # eval $valuecmd "{$value}"
               # error "Warning: Control widgets should specify initial value"
            }
         }
         if { $command != "" } {
            regsub %v $command "{$value}" com
            eval $com 
         }
      }


#-----------------------------------------------------------------------
      public variable state {} {
#-----------------------------------------------------------------------
         if { $state == "disabled" || $balloonstr == "" } {
            $itk_component(toolbar) configure -balloondelay1 999999
            $itk_component(toolbar) configure -balloondelay2 999999
         } elseif { $state == "normal" } {
            $itk_component(toolbar) configure -balloondelay1 1200
            $itk_component(toolbar) configure -balloondelay2 1200
         }
      }


#-----------------------------------------------------------------------
      public variable valuevar {} {
#-----------------------------------------------------------------------
         if { $valuevar != "" } {

#  Find the level at which we are being called from - the level which 
#  contains this variable as a public variable.
            set level 0
            for { set l [ info level ] } { $l > 0 } { incr l -1 } {
               if { ! [ catch { uplevel #$l set $valuevar } ] } {
                  set level $l
                  break
               }
            }
            if { $level <= 0 } {
               error "Cannot find variable $valuevar in stack"
            }

            set valuecmd [ uplevel #$level code \$this configure -$valuevar ]
            upvar #$level $valuevar var
            set var $value
            set valuevarname [ uplevel #$level scope $valuevar ]
            trace variable var w [ code $this valuevarchange ]
         } else {
            set valuecmd ""
         }
      }


########################################################################
#  Private methods.                                                    #
########################################################################
      private method valuevarchange { name1 name2 op } {
         configure -value [ set $valuevarname ]
      }


########################################################################
#  Private variables.                                                  #
########################################################################
      private variable childsite        ;# Path of child site
      private variable control          ;# Pathname of the control widget
      private variable valuecmd ""      ;# Command to execute on value change
      private variable valuevarname     ;# Scoped name of value variable

  }


########################################################################
#  Widget resource management
########################################################################

   itk::usual Control {
      keep -background -cursor -foreground
   }


########################################################################
#  Constructor alias
########################################################################

   proc control { pathname args } {
      uplevel Control $pathname $args
   }
   

# $Id$
